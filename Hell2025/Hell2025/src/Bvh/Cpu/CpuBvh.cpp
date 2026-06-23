#include "CpuBvh.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include <Game/Constants.h>
#include <Game/UniqueID.h>
#include "Hell/BVH/BVH.h"
#include "Util.h"
#include "Timer.hpp"

#include <iostream>
#include <unordered_map>

#include "bvh/v2/bvh.h"
#include "bvh/v2/vec.h"
#include "bvh/v2/bbox.h"
#include "bvh/v2/ray.h"
#include "bvh/v2/node.h"
#include "bvh/v2/default_builder.h"
#include "bvh/v2/thread_pool.h"
#include "bvh/v2/stack.h"

using MadmannVec3 = bvh::v2::Vec<float, 3>;
using MadmannBBox = bvh::v2::BBox<float, 3>;
using MadmannBvhNode = bvh::v2::Node<float, 3>;
using MadmannBvh = bvh::v2::Bvh<MadmannBvhNode>;
using MadmannBvhBuilder = bvh::v2::DefaultBuilder<MadmannBvhNode>;
using MadmannBvhBuilderConfig = typename bvh::v2::DefaultBuilder<MadmannBvhNode>::Config;

namespace Bvh::Cpu {
	std::unordered_map<uint64_t, MeshBvh> g_meshBvhs;
	std::unordered_map<uint64_t, SceneBvh> g_sceneBvhs;
	std::unordered_map<uint64_t, MadmannBvh> g_madmannSceneBvhs; // mapped to scene ids


	bvh::v2::ThreadPool g_threadPool;

	// Gpu data
	std::unordered_map<uint64_t, uint32_t> g_meshBvhRootNodeOffsetMapping; // Maps a flatterend MeshBvh's root node to its id
	std::vector<BvhNode> g_meshBvhsNodes;
	std::vector<BVHTriangle> g_triangles;

	glm::vec3 BvhVec3ToGlmVec3(MadmannVec3 vec);
	MadmannVec3 GlmVec3ToBvhVec3(glm::vec3 vec);

	uint64_t CreateMeshBvhFromMeshBvh(MeshBvh& sourceMeshBvh) {
		uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!

		MeshBvh& targetMeshBvh = g_meshBvhs[uniqueId];
		targetMeshBvh.m_nodes.swap(sourceMeshBvh.m_nodes);
		targetMeshBvh.m_triangles.swap(sourceMeshBvh.m_triangles);
		return uniqueId;
	}

	uint64_t CreateMeshBvhFromVertexData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
		uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!
		g_meshBvhs[uniqueId] = Hell::Bvh::BuildMeshBvh(vertices, indices);
		return uniqueId;
	}


	uint64_t CreateNewSceneBvh() {
		uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!
		g_sceneBvhs[uniqueId] = SceneBvh();
		g_madmannSceneBvhs[uniqueId] = MadmannBvh();

		return uniqueId;
	}


	void UpdateSceneBvh(uint64_t bvhId, std::vector<PrimitiveInstance>& instances) {
		// Early out if bvhId is invalid
		if (!SceneBvhExists(bvhId)) return;

		SceneBvh& sceneBvh = g_sceneBvhs[bvhId];
		MadmannBvh& bvh = g_madmannSceneBvhs[bvhId];

		// Clear last frames data
		sceneBvh.m_gpuInstances.clear();
		sceneBvh.m_nodes.clear();

		// Check if refit is possible
		bool refitPossible = true;

		if (instances.size() == sceneBvh.m_instances.size()) {
			for (size_t i = 0; i < instances.size(); ++i) {
				if (instances[i].meshBvhId != sceneBvh.m_instances[i].meshBvhId) {
					refitPossible = false;
					break;
				}
			}
		}
		else {
			refitPossible = false;
		}

		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?
		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?
		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?
		refitPossible = false;
		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?
		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?
		// REFIT DOESN'T APPEAR TO WORK!!! Perhaps you need more than just new bounding boxes, like centers?

		// Copy the cpu instances (for refitting)
		sceneBvh.m_instances = instances;

		// Bail if no work to do
		if (instances.empty()) return;

		// Create new bvh
		if (!refitPossible) {
			std::vector<MadmannBBox> bboxes(instances.size());
			std::vector<MadmannVec3> centers(instances.size());

			for (int i = 0; i < instances.size(); i++) {
				PrimitiveInstance& instance = instances[i];
				bboxes[i] = MadmannBBox(
					*reinterpret_cast<const MadmannVec3*>(&instance.worldAabbBoundsMin),
					*reinterpret_cast<const MadmannVec3*>(&instance.worldAabbBoundsMax));
				centers[i] = *reinterpret_cast<const MadmannVec3*>(&instance.worldAabbCenter);
			}

			MadmannBvhBuilderConfig config;
			config.quality = bvh::v2::DefaultBuilder<MadmannBvhNode>::Quality::High;

			bvh = bvh::v2::DefaultBuilder<MadmannBvhNode>::build(g_threadPool, bboxes, centers, config);
		}
		// Refit existing bvh
		else {
			bvh.refit([&](MadmannBvhNode& node) {
				size_t prim_id_in_list = bvh.prim_ids[node.index.first_id()];
				const PrimitiveInstance& instance = instances[prim_id_in_list];
				MadmannBBox newBbox = MadmannBBox(
					*reinterpret_cast<const MadmannVec3*>(&instance.worldAabbBoundsMin),
					*reinterpret_cast<const MadmannVec3*>(&instance.worldAabbBoundsMax));
				node.set_bbox(newBbox);
				});
		}

		// Create custom node structure
		int nodeCount = bvh.nodes.size();
		sceneBvh.m_nodes.resize(nodeCount);

		for (int i = 0; i < nodeCount; i++) {
			const MadmannBvhNode& mmNode = bvh.nodes[i];
			BvhNode& node = sceneBvh.m_nodes[i];

			float* minPtr = glm::value_ptr(node.boundsMin);
			minPtr[0] = mmNode.bounds[0]; // X_min
			minPtr[1] = mmNode.bounds[2]; // Y_min
			minPtr[2] = mmNode.bounds[4]; // Z_min

			float* maxPtr = glm::value_ptr(node.boundsMax);
			maxPtr[0] = mmNode.bounds[1]; // X_max
			maxPtr[1] = mmNode.bounds[3]; // Y_max
			maxPtr[2] = mmNode.bounds[5]; // Z_max

			node.primitiveCount = mmNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
			node.firstChildOrPrimitive = mmNode.index.value >> MadmannBvhNode::prim_count_bits;
		}

		// Walk the scene BVH and create the GPU primitive instance array, specifically ordering them such that leaf node instances are adjacent
		sceneBvh.m_gpuInstances.reserve(instances.size());
		uint32_t stack[MAX_BVH_STACK_SIZE];
		size_t stack_size = 0;
		uint32_t rootNodeIndex = 0;
		stack[stack_size++] = rootNodeIndex;

		while (stack_size != 0) {
			uint32_t currentIndex = stack[--stack_size];
			BvhNode& node = sceneBvh.m_nodes[currentIndex];

			// If this node is a leaf...
			if (node.primitiveCount > 0) {
				int newPrimitiveIndex = static_cast<int>(sceneBvh.m_gpuInstances.size());

				// Loop over each primitive instance in the leaf
				for (uint32_t i = 0; i < node.primitiveCount; i++) {
					int primitiveId = node.firstChildOrPrimitive + i;
					int instanceIndex = bvh.prim_ids[primitiveId];

					const PrimitiveInstance& instance = instances[instanceIndex];

					// Create the GPU primitive instance
					GpuPrimitiveInstance& gpuInstance = sceneBvh.m_gpuInstances.emplace_back();
					gpuInstance.rootNodeIndex = g_meshBvhRootNodeOffsetMapping[instance.meshBvhId];
					gpuInstance.worldTransform = instance.worldTransform;
					gpuInstance.inverseWorldTransform = instance.inverseWorldTransform;
					gpuInstance.openableId = instance.openableId;
					gpuInstance.customId = instance.customId;
					gpuInstance.globalMeshIndex = instance.globalMeshIndex;
					gpuInstance.localMeshNodeIndex = instance.localMeshNodeIndex;
					Util::PackUint64(instance.objectId, gpuInstance.objectIdLowerBit, gpuInstance.objectIdUpperBit);
				}
				// Update the leaf node's pointer so it now points into the new, contiguous instance array
				node.firstChildOrPrimitive = newPrimitiveIndex;
			}
			else {
				stack[stack_size++] = node.firstChildOrPrimitive;
				stack[stack_size++] = node.firstChildOrPrimitive + 1;
			}
		}
	}

	void FlatternMeshBvhNodes() {
		g_meshBvhRootNodeOffsetMapping.clear();
		g_meshBvhsNodes.clear();
		g_triangles.clear();

		// Preallocate memory
		size_t totalNodeCount = 0;
		size_t totalTriangleCount = 0;

		for (auto it = g_meshBvhs.begin(); it != g_meshBvhs.end(); ++it) {
			MeshBvh& meshBvh = it->second;
			totalNodeCount += meshBvh.m_nodes.size();
			totalTriangleCount += meshBvh.m_triangles.size();
		}
		g_meshBvhsNodes.reserve(totalNodeCount);
		g_triangles.reserve(totalTriangleCount);

		uint32_t rootNodeOffset = 0;
		uint32_t baseTriangleFloatOffset = 0;

		// Iterate each mesh bvh, and store its nodes and triangle data in the global ararys
		for (auto it = g_meshBvhs.begin(); it != g_meshBvhs.end(); ++it) {
			uint64_t bvhId = it->first;
			MeshBvh& meshBvh = it->second;

			// Store the root node and triangle offsets
			g_meshBvhRootNodeOffsetMapping[bvhId] = rootNodeOffset;

			// Append this mesh's nodes to the global vector
			for (BvhNode& node : meshBvh.m_nodes) {

				// Copy the node
				BvhNode& appendedNode = g_meshBvhsNodes.emplace_back(node);

				// If the node is a leaf, add the base triangle offset
				if (appendedNode.primitiveCount > 0) {
					appendedNode.firstChildOrPrimitive += baseTriangleFloatOffset;
				}
				// If it's not a leaf, then add the root node offset
				else {
					appendedNode.firstChildOrPrimitive += rootNodeOffset;
				}
			}

			// Copy the triangle data from this mesh into the global vector
			g_triangles.insert(g_triangles.end(), meshBvh.m_triangles.begin(), meshBvh.m_triangles.end());

			// Increment offsets
			rootNodeOffset += (uint32_t)meshBvh.m_nodes.size();
			baseTriangleFloatOffset += static_cast<uint32_t>(meshBvh.m_triangles.size() * 12);
		}
	}

	glm::vec3 BvhVec3ToGlmVec3(MadmannVec3 vec) {
		return { vec.values[0], vec.values[1], vec.values[2] };
	}

	MadmannVec3 GlmVec3ToBvhVec3(glm::vec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	bool MeshBvhExists(uint64_t bvhId) {
		return (g_meshBvhs.find(bvhId) != g_meshBvhs.end());
	}

	bool SceneBvhExists(uint64_t bvhId) {
		return (g_sceneBvhs.find(bvhId) != g_sceneBvhs.end());
	}

	void DestroySceneBvh(uint64_t bvhId) {
		auto it = g_sceneBvhs.find(bvhId);
		if (it != g_sceneBvhs.end()) {
			g_sceneBvhs.erase(it);
		}
	}

	void DestroyMeshBvh(uint64_t bvhId) {
		auto it = g_meshBvhs.find(bvhId);
		if (it != g_meshBvhs.end()) {
			g_meshBvhs.erase(it);
		}
	}

	SceneBvh* GetSceneBvhById(uint64_t bvhId) {
		if (!SceneBvhExists(bvhId)) return nullptr;
		return &g_sceneBvhs[bvhId];
	}

	MeshBvh* GetMeshBvhById(uint64_t bvhId) {
		if (!MeshBvhExists(bvhId)) return nullptr;
		return &g_meshBvhs[bvhId];
	}

	const std::vector<BvhNode>& GetMeshGpuBvhNodes() {
		return g_meshBvhsNodes;
	}

	//const std::vector<BvhNode>& GetSceneGpuBvhNodes() {
	//    return g_sceneBvhsNodes;
	//}

	const std::vector<BVHTriangle>& GetTriangles() {
		return g_triangles;
	}

	const std::vector<GpuPrimitiveInstance>& GetGpuEntityInstances(uint64_t sceneBvhId) {
		static std::vector<GpuPrimitiveInstance> empty;

		if (!SceneBvhExists(sceneBvhId)) return empty;
		return g_sceneBvhs[sceneBvhId].m_gpuInstances;
	}
}
