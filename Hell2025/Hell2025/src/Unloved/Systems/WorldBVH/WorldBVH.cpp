#include "WorldBVH.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Common/Bit.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/Math.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/DDGI/DDGIGeometryBuilder.h"
#include "Unloved/World/World.h"
#include "Timer.hpp"

#include <iostream>

namespace Unloved::WorldBVH {

	std::vector<PrimitiveInstance> g_dynamicSceneInstances;
	std::vector<PrimitiveInstance> g_staticSceneInstances;
	uint64_t g_dynamicSceneBvhId = 0;
	uint64_t g_staticSceneBvhId = 0;
    uint64_t g_houseLightOccluderMeshBvhId = 0;
    uint64_t g_houseLightOccluderSceneBvhId = 0;
	bool g_staticBvhSceneDirty = true;

	void DebugDraw();
	void UpdateDynamicBvhScene();
	void UpdateStaticBvhScene();

	void UpdateBvhs() {
		if (g_staticBvhSceneDirty) {
			g_staticBvhSceneDirty = false;
			UpdateStaticBvhScene();
		}

		UpdateDynamicBvhScene();
		//DebugDraw();
	}

    void CreateObjectInstanceDataFromRenderItem(const RenderItem& renderItem, std::vector<PrimitiveInstance>& container) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
        if (!mesh) return;

        PrimitiveInstance& instance = container.emplace_back();
		instance.worldTransform = renderItem.modelMatrix;
		instance.inverseWorldTransform = renderItem.inverseModelMatrix;
        instance.worldAabbBoundsMin = renderItem.aabbMin;
        instance.worldAabbBoundsMax = renderItem.aabbMax;
        instance.worldAabbCenter = (renderItem.aabbMin + renderItem.aabbMax) * 0.5f;
        instance.meshBvhId = mesh->meshBvhId;
        instance.openableId= renderItem.openableId;
        instance.globalMeshIndex = renderItem.meshId;
        instance.customId = renderItem.customId;
        instance.localMeshNodeIndex = renderItem.localMeshNodeIndex;
        Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, instance.objectId);
    }

	void CreateObjectInstanceDataFromRenderItems(const std::vector<RenderItem>& renderItems, std::vector<PrimitiveInstance>& container) {
        for (const RenderItem& renderItem : renderItems) {
            CreateObjectInstanceDataFromRenderItem(renderItem, container);
        }
    }

    void RebuildHouseLightOcclusionBvh() {
        Hell::Bvh::DestroyMeshBvh(g_houseLightOccluderMeshBvhId);
        Hell::Bvh::DestroySceneBvh(g_houseLightOccluderSceneBvhId);
        g_houseLightOccluderMeshBvhId = 0;
        g_houseLightOccluderSceneBvhId = 0;

        const glm::vec3 boundsMin = glm::vec3(-999.0f);
        const glm::vec3 boundsMax = glm::vec3(999.0f);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        DDGIGeometryBuilder::BuildHouseMesh(boundsMin, boundsMax, vertices, indices);

        if (vertices.empty() || indices.empty()) {
            return;
        }

        g_houseLightOccluderMeshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
        const MeshBvh* meshBvh = Hell::Bvh::GetMeshBvhById(g_houseLightOccluderMeshBvhId);
        if (!meshBvh || meshBvh->m_nodes.empty()) {
            Hell::Bvh::DestroyMeshBvh(g_houseLightOccluderMeshBvhId);
            g_houseLightOccluderMeshBvhId = 0;
            return;
        }

        g_houseLightOccluderSceneBvhId = Hell::Bvh::CreateSceneBvh();
        if (!Hell::Bvh::AddMeshBvhToSceneBvh(g_houseLightOccluderSceneBvhId, g_houseLightOccluderMeshBvhId)) {
            Hell::Bvh::DestroySceneBvh(g_houseLightOccluderSceneBvhId);
            Hell::Bvh::DestroyMeshBvh(g_houseLightOccluderMeshBvhId);
            g_houseLightOccluderSceneBvhId = 0;
            g_houseLightOccluderMeshBvhId = 0;
            return;
        }

        const BvhNode& rootNode = meshBvh->m_nodes[0];
        std::vector<PrimitiveInstance> instances;
        PrimitiveInstance& instance = instances.emplace_back(PrimitiveInstance{});
        instance.meshBvhId = g_houseLightOccluderMeshBvhId;
        instance.worldAabbBoundsMin = rootNode.boundsMin;
        instance.worldAabbBoundsMax = rootNode.boundsMax;
        instance.worldAabbCenter = (rootNode.boundsMin + rootNode.boundsMax) * 0.5f;
        instance.worldTransform = glm::mat4(1.0f);
        instance.inverseWorldTransform = glm::mat4(1.0f);

        if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_houseLightOccluderSceneBvhId)) {
            sceneBvh->UpdateInstances(instances);
        }
    }

	void CreatePrimtiveInstanceFromMeshNode(const MeshNode* meshNode, std::vector<PrimitiveInstance>& container) {
        if (!meshNode) return;

		PrimitiveInstance& instance = container.emplace_back();
		instance.worldTransform = meshNode->worldMatrix;
		instance.inverseWorldTransform = meshNode->inverseWorldMatrix;
		instance.worldAabbBoundsMin = meshNode->worldspaceAabb.GetBoundsMin();
		instance.worldAabbBoundsMax = meshNode->worldspaceAabb.GetBoundsMax();
		instance.worldAabbCenter = meshNode->worldspaceAabb.GetCenter();
		instance.meshBvhId = meshNode->meshBvhId;
		instance.openableId = meshNode->openableId;
		instance.globalMeshIndex = meshNode->globalMeshIndex;
		instance.customId = meshNode->customId;
        instance.localMeshNodeIndex = meshNode->nodeIndex;
        instance.objectId = meshNode->parentObjectId;
	}

	void DebugDraw() {
		for (PrimitiveInstance& primitiveInstance : g_dynamicSceneInstances) {
			DebugDraw::DrawAABB(AABB(primitiveInstance.worldAabbBoundsMin, primitiveInstance.worldAabbBoundsMax), YELLOW);
		}
		for (PrimitiveInstance& primitiveInstance : g_staticSceneInstances) {
			DebugDraw::DrawAABB(AABB(primitiveInstance.worldAabbBoundsMin, primitiveInstance.worldAabbBoundsMax), GREEN);
		}
	}

	void CreateDynamicPrimtiveInstances(MeshNodes& meshNodes) {
		for (int i = 0; i < meshNodes.GetNodeCount(); i++) {
			MeshNode* meshNode = meshNodes.GetMeshNodeByLocalIndex(i);
			if (!meshNodes.MeshNodeIsStatic(i)) {
				CreatePrimtiveInstanceFromMeshNode(meshNode, g_dynamicSceneInstances);
			}
		}
	}

    void GatherDynamicMeshNodeInstances() {
        for (Door& door : Unloved::World::GetDoors()) {
            CreateDynamicPrimtiveInstances(door.GetMeshNodes());
        }
        for (Fireplace& fireplace : Unloved::World::GetFireplaces()) {
            CreateDynamicPrimtiveInstances(fireplace.GetMeshNodes());
        }
        for (GenericObject& genericObject : Unloved::World::GetGenericObjects()) {
            CreateDynamicPrimtiveInstances(genericObject.GetMeshNodes());
        }
        for (Piano& piano : Unloved::World::GetPianos()) {
            CreateDynamicPrimtiveInstances(piano.GetMeshNodes());
        }
    }

    void GatherDynamicRenderItemInstances() {
        for (PictureFrame& pictureFrame : Unloved::World::GetPictureFrames()) {
            CreateObjectInstanceDataFromRenderItems(pictureFrame.GetRenderItems(), g_dynamicSceneInstances);
        }
        for (PickUp& pickUp : Unloved::World::GetPickUps()) {
            CreateObjectInstanceDataFromRenderItems(pickUp.GetRenderItems(), g_dynamicSceneInstances);
        }
    }

    void CreateStaticPrimtiveInstances(MeshNodes& meshNodes) {
		for (int i = 0; i < meshNodes.GetNodeCount(); i++) {
			MeshNode* meshNode = meshNodes.GetMeshNodeByLocalIndex(i);
			if (meshNodes.MeshNodeIsStatic(i)) {
				CreatePrimtiveInstanceFromMeshNode(meshNode, g_staticSceneInstances);
			}
		}
	}

	void UpdateDynamicBvhScene() {
		//Timer timer("DynamicBvhSceneUpdate");

		// Create scene if it doesn't exist
		if (g_dynamicSceneBvhId == 0) {
            g_dynamicSceneBvhId = Hell::Bvh::CreateSceneBvh();
		}

		// Clear any existing primitive instances
		g_dynamicSceneInstances.clear();

        GatherDynamicMeshNodeInstances();
        GatherDynamicRenderItemInstances();

		// Recreate the TLAS
		if (!Hell::Bvh::AddInstanceMeshBvhsToSceneBvh(g_dynamicSceneBvhId, g_dynamicSceneInstances)) return;
        if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_dynamicSceneBvhId)) {
            sceneBvh->UpdateInstances(g_dynamicSceneInstances);
        }
	}

	void UpdateStaticBvhScene() {
        // Create scene if it doesn't exist
		if (g_staticSceneBvhId == 0) {
			g_staticSceneBvhId = Hell::Bvh::CreateSceneBvh();
		}

        // Clear any existing primitive instances
		g_staticSceneInstances.clear();

        // Render items
        for (ChristmasLightSet& object : World::GetChristmasLightSets())	CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Fence& object : World::GetFences())							CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Jetty& object : World::GetJetties())							CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Ladder& object : World::GetLadders())							CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Mermaid& object : World::GetMermaids())						CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (PowerPoleSet& object : World::GetPowerPoleSets())			    CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Staircase& object : World::GetStaircases())					CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);

        // Add any static mesh nodes to the primitive instances vector
        for (Door& object : World::GetDoors())                     CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Fireplace& object : World::GetFireplaces())           CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (GenericObject& object : World::GetGenericObjects())   CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Light& object : World::GetLights())				   CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Piano& object : World::GetPianos())                   CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Window& object : World::GetWindows())                 CreateStaticPrimtiveInstances(object.GetMeshNodes());

        // Recreate the TLAS
		if (!Hell::Bvh::AddInstanceMeshBvhsToSceneBvh(g_staticSceneBvhId, g_staticSceneInstances)) return;
        if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_staticSceneBvhId)) {
            sceneBvh->UpdateInstances(g_staticSceneInstances);
        }
		std::cout << "Updated static scene Bvh\n";
    }

	BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
		// Bail if invalid ray direction
        if (Hell::Math::IsNan(rayDir)) {
			return BvhRayResult();
        }

		// First check for a hit with the static scene
        BvhRayResult staticResult;
        staticResult.distanceToHit = maxRayDistance;
        if (SceneBvh* staticSceneBvh = Hell::Bvh::GetSceneBvhById(g_staticSceneBvhId)) {
            staticResult = staticSceneBvh->ClosestHit(rayOrigin, rayDir, maxRayDistance);
        }

        // If a hit was found, then update the max ray distance so you don't search further than you need to in the dynamic scene raycast
        if (staticResult.hitFound) {
            maxRayDistance = staticResult.distanceToHit;
        }

        BvhRayResult dynamicResult;
        dynamicResult.distanceToHit = maxRayDistance;
        if (SceneBvh* dynamicSceneBvh = Hell::Bvh::GetSceneBvhById(g_dynamicSceneBvhId)) {
            dynamicResult = dynamicSceneBvh->ClosestHit(rayOrigin, rayDir, maxRayDistance);
        }

        // Dynamic scene hit was closest
        if (dynamicResult.hitFound) {
            return dynamicResult;
        }
        // Otherwise return the static result, which may or may not be a hit
        else {
            return staticResult;
        }
	}

    BvhRayResult ClosestHouseLightOcclusionHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
        BvhRayResult rayResult;
        rayResult.distanceToHit = maxRayDistance;

        if (Hell::Math::IsNan(rayDir)) {
            return rayResult;
        }

        if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_houseLightOccluderSceneBvhId)) {
            return sceneBvh->ClosestHit(rayOrigin, rayDir, maxRayDistance);
        }

        return rayResult;
    }

	void MarkStaticSceneBvhDirty() {
		g_staticBvhSceneDirty = true;
	}
}
