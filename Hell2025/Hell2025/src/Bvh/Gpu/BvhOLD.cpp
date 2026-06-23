#include "BvhOLD.h"
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

#include "Hell/Logging.h"

using MadmannVec3 = bvh::v2::Vec<float, 3>;
using MadmannBBox = bvh::v2::BBox<float, 3>;
using MadmannBvhNode = bvh::v2::Node<float, 3>;
using MadmannBvh = bvh::v2::Bvh<MadmannBvhNode>;
using MadmannBvhBuilder = bvh::v2::DefaultBuilder<MadmannBvhNode>;

namespace Bvh::Gpu {
    std::unordered_map<uint64_t, MeshBvh> g_meshBvhs;
    std::unordered_map<uint64_t, SceneBvh> g_sceneBvhs;
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
        SceneBvh& sceneBvh = g_sceneBvhs[uniqueId];
        return uniqueId;
    }

    void UpdateSceneBvh(uint64_t bvhId, std::vector<PrimitiveInstance>& instances) {
        //Timer timer("UpdateSceneBvh() " + std::to_string(instances.size()) + " instances");

        // Early out if bvhId is invalid
        if (!SceneBvhExists(bvhId)) return;

        SceneBvh& sceneBvh = g_sceneBvhs[bvhId];

        // Clear last frames data
        sceneBvh.m_gpuInstances.clear();
        sceneBvh.m_nodes.clear();

        if (instances.empty()) return;

        std::vector<MadmannBBox> bboxes(instances.size());
        std::vector<MadmannVec3> centers(instances.size());

        for (int i = 0; i < instances.size(); i++) {
            PrimitiveInstance& instance = instances[i];
            MadmannVec3 aabbMin = GlmVec3ToBvhVec3(instance.worldAabbBoundsMin);
            MadmannVec3 aabbMax = GlmVec3ToBvhVec3(instance.worldAabbBoundsMax);
            MadmannVec3 center = GlmVec3ToBvhVec3(instance.worldAabbCenter);
            bboxes[i] = MadmannBBox(aabbMin, aabbMax);
            centers[i] = center;
        }

        typename bvh::v2::DefaultBuilder<MadmannBvhNode>::Config config;
        config.quality = bvh::v2::DefaultBuilder<MadmannBvhNode>::Quality::High;
        MadmannBvh bvh = bvh::v2::DefaultBuilder<MadmannBvhNode>::build(g_threadPool, bboxes, centers, config);

        int nodeCount = bvh.nodes.size();
        sceneBvh.m_nodes.resize(nodeCount);

        for (int i = 0; i < nodeCount; i++) {
            const MadmannBvhNode& mmNode = bvh.nodes[i];
            BvhNode& node = sceneBvh.m_nodes[i];
            node.boundsMin = glm::vec3(mmNode.bounds[0], mmNode.bounds[2], mmNode.bounds[4]);
            node.boundsMax = glm::vec3(mmNode.bounds[1], mmNode.bounds[3], mmNode.bounds[5]);
            node.primitiveCount = mmNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
            node.firstChildOrPrimitive = mmNode.index.value >> MadmannBvhNode::prim_count_bits;
        }

        sceneBvh.m_gpuInstances.reserve(instances.size());

        // Walk the scene BVH and create the entity instance array, specifically ordering them such that leaf node instances are adjacent
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
                    gpuInstance.inverseWorldTransform = glm::inverse(gpuInstance.worldTransform);
                    gpuInstance.openableId = instance.openableId;
                    gpuInstance.customId = instance.customId;
                    gpuInstance.globalMeshIndex = instance.globalMeshIndex;
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
        uint32_t totalNodeCount = 0;
        uint32_t totalTriangleCount = 0;
        for (auto it = g_meshBvhs.begin(); it != g_meshBvhs.end(); ++it) {
            MeshBvh& meshBvh = it->second;
            totalNodeCount += (uint32_t)meshBvh.m_nodes.size();
            totalTriangleCount += static_cast<uint32_t>(meshBvh.m_triangles.size());
        }
        g_meshBvhsNodes.reserve(totalNodeCount);
        g_triangles.reserve(totalTriangleCount);

        uint32_t rootNodeOffset = 0;
        uint32_t baseTriangleFloatOffset = 0;

        // Iterate each mesh bvh, and store its nodes and triangle data in the global arrays
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

    glm::vec3 GetMeshBvhRootNodeBoundsMin(uint64_t bvhId) {
        MeshBvh* meshBvh = GetMeshBvhById(bvhId);
        if (!meshBvh || meshBvh->m_nodes.empty()) {
            Logging::Fatal() << "Bvh::Gpu::GetMeshBvhRootNodeBoundsMin() failed: invalid or empty mesh BVH\n";
            return glm::vec3(0.0f);
        }

        return meshBvh->m_nodes[0].boundsMin;
    }

    glm::vec3 GetMeshBvhRootNodeBoundsMax(uint64_t bvhId) {
        MeshBvh* meshBvh = GetMeshBvhById(bvhId);
        if (!meshBvh || meshBvh->m_nodes.empty()) {
            Logging::Fatal() << "Bvh::Gpu::GetMeshBvhRootNodeBoundsMax() failed: invalid or empty mesh BVH\n";
            return glm::vec3(0.0f);
        }

        return meshBvh->m_nodes[0].boundsMax;
    }
}
