#include "WorldBVH.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Common/Bit.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/Math.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Material.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/House/WorldPlane.h"
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

    bool TriangleIntersectsBounds(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        const glm::vec3 triangleBoundsMin = glm::min(glm::min(p0, p1), p2);
        const glm::vec3 triangleBoundsMax = glm::max(glm::max(p0, p1), p2);

        return
            triangleBoundsMax.x >= boundsMin.x && triangleBoundsMin.x <= boundsMax.x &&
            triangleBoundsMax.y >= boundsMin.y && triangleBoundsMin.y <= boundsMax.y &&
            triangleBoundsMax.z >= boundsMin.z && triangleBoundsMin.z <= boundsMax.z;
    }

    void AddHouseOccluderTriangle(std::vector<HouseOccluderTriangle>& triangles, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, int baseColorTextureIndex, int rmaTextureIndex) {
        const glm::vec3 edge1 = p1 - p0;
        const glm::vec3 edge2 = p2 - p0;
        const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        HouseOccluderTriangle& triangle = triangles.emplace_back();
        triangle.v0 = p0;
        triangle.v1 = p1;
        triangle.v2 = p2;
        triangle.uv0 = uv0;
        triangle.uv1 = uv1;
        triangle.uv2 = uv2;
        triangle.normal = normal;
        triangle.baseColorTextureIndex = baseColorTextureIndex;
        triangle.rmaTextureIndex = rmaTextureIndex;
    }

    void AddHouseOccluderTriangleIfInsideBounds(std::vector<HouseOccluderTriangle>& triangles, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, int baseColorTextureIndex, int rmaTextureIndex, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        if (!TriangleIntersectsBounds(p0, p1, p2, boundsMin, boundsMax)) {
            return;
        }

        AddHouseOccluderTriangle(triangles, p0, p1, p2, uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex);
    }

    void AddWorldPlaneOccluderTriangles(std::vector<HouseOccluderTriangle>& triangles, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        for (WorldPlane& plane : Unloved::World::GetWorldPlanes()) {
            if (plane.GetParentDoorId() != 0) {
                continue;
            }

            const std::vector<Vertex>& planeVertices = plane.GetVertices();
            const std::vector<uint32_t>& planeIndices = plane.GetIndices();
            Material* material = plane.GetMaterial();
            const int baseColorTextureIndex = material ? material->m_basecolor : -1;
            const int rmaTextureIndex = material ? material->m_rma : -1;

            for (uint32_t i = 0; i + 2 < planeIndices.size(); i += 3) {
                const uint32_t idx0 = planeIndices[i + 0];
                const uint32_t idx1 = planeIndices[i + 1];
                const uint32_t idx2 = planeIndices[i + 2];

                if (idx0 >= planeVertices.size() || idx1 >= planeVertices.size() || idx2 >= planeVertices.size()) {
                    continue;
                }

                AddHouseOccluderTriangleIfInsideBounds(triangles, planeVertices[idx0].position, planeVertices[idx1].position, planeVertices[idx2].position, planeVertices[idx0].uv, planeVertices[idx1].uv, planeVertices[idx2].uv, baseColorTextureIndex, rmaTextureIndex, boundsMin, boundsMax);
            }
        }
    }

    void AddWallOccluderTriangles(std::vector<HouseOccluderTriangle>& triangles, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        for (Wall& wall : Unloved::World::GetWalls()) {
            Material* material = wall.GetMaterial();
            const int baseColorTextureIndex = material ? material->m_basecolor : -1;
            const int rmaTextureIndex = material ? material->m_rma : -1;

            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                const std::vector<Vertex>& wallVertices = wallSegment.GetVertices();
                const std::vector<uint32_t>& wallIndices = wallSegment.GetIndices();

                for (uint32_t i = 0; i + 2 < wallIndices.size(); i += 3) {
                    const uint32_t idx0 = wallIndices[i + 0];
                    const uint32_t idx1 = wallIndices[i + 1];
                    const uint32_t idx2 = wallIndices[i + 2];

                    if (idx0 >= wallVertices.size() || idx1 >= wallVertices.size() || idx2 >= wallVertices.size()) {
                        continue;
                    }

                    AddHouseOccluderTriangleIfInsideBounds(triangles, wallVertices[idx0].position, wallVertices[idx1].position, wallVertices[idx2].position, wallVertices[idx0].uv, wallVertices[idx1].uv, wallVertices[idx2].uv, baseColorTextureIndex, rmaTextureIndex, boundsMin, boundsMax);
                }
            }
        }
    }

    void AddDoorGapOccluderTriangles(std::vector<HouseOccluderTriangle>& triangles, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        Material* material = Hell::ResourceManager::GetMaterialByName("Ceiling2");
        const int baseColorTextureIndex = material ? material->m_basecolor : -1;
        const int rmaTextureIndex = material ? material->m_rma : -1;

        for (Door& door : Unloved::World::GetDoors()) {
            const glm::mat4& modelMatrix = door.GetDoorModelMatrix();

            const float padding = 0.02f;
            const float halfP = padding * 0.5f;
            const float halfD = DOOR_WIDTH * 0.5f + halfP;
            const float h = DOOR_HEIGHT + halfP;
            const float halfW = 0.05f;

            glm::vec3 p[8];
            p[0] = glm::vec3(halfW, 0.0f, halfD);
            p[1] = glm::vec3(-halfW, 0.0f, halfD);
            p[2] = glm::vec3(-halfW, h, halfD);
            p[3] = glm::vec3(halfW, h, halfD);
            p[4] = glm::vec3(halfW, 0.0f, -halfD);
            p[5] = glm::vec3(-halfW, 0.0f, -halfD);
            p[6] = glm::vec3(-halfW, h, -halfD);
            p[7] = glm::vec3(halfW, h, -halfD);

            for (int i = 0; i < 8; ++i) {
                p[i] = glm::vec3(modelMatrix * glm::vec4(p[i], 1.0f));
            }

            auto addFace = [&](int i0, int i1, int i2, int i3) {
                const glm::vec2 uv0 = glm::vec2(0.0f, 0.0f);
                const glm::vec2 uv1 = glm::vec2(1.0f, 0.0f);
                const glm::vec2 uv2 = glm::vec2(1.0f, 1.0f);
                const glm::vec2 uv3 = glm::vec2(0.0f, 1.0f);

                AddHouseOccluderTriangleIfInsideBounds(triangles, p[i0], p[i1], p[i2], uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex, boundsMin, boundsMax);
                AddHouseOccluderTriangleIfInsideBounds(triangles, p[i2], p[i3], p[i0], uv2, uv3, uv0, baseColorTextureIndex, rmaTextureIndex, boundsMin, boundsMax);
            };

            addFace(0, 1, 2, 3);
            addFace(5, 4, 7, 6);
            addFace(3, 2, 6, 7);
            addFace(1, 0, 4, 5);
        }
    }

    void CreateHouseOccluderTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<HouseOccluderTriangle>& triangles) {
        triangles.clear();

        AddWorldPlaneOccluderTriangles(triangles, boundsMin, boundsMax);
        AddWallOccluderTriangles(triangles, boundsMin, boundsMax);
        AddDoorGapOccluderTriangles(triangles, boundsMin, boundsMax);
    }

    void CreateHouseOccluderGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        std::vector<HouseOccluderTriangle> triangles;
        CreateHouseOccluderTriangles(boundsMin, boundsMax, triangles);

        vertices.clear();
        indices.clear();
        vertices.reserve(triangles.size() * 3);
        indices.reserve(triangles.size() * 3);

        for (const HouseOccluderTriangle& triangle : triangles) {
            const uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

            Vertex v0 = {};
            Vertex v1 = {};
            Vertex v2 = {};
            v0.position = triangle.v0;
            v1.position = triangle.v1;
            v2.position = triangle.v2;
            v0.normal = triangle.normal;
            v1.normal = triangle.normal;
            v2.normal = triangle.normal;
            v0.uv = triangle.uv0;
            v1.uv = triangle.uv1;
            v2.uv = triangle.uv2;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
        }
    }

    void UpdateHouseLightOccluderBvh() {
        Hell::Bvh::DestroyMeshBvh(g_houseLightOccluderMeshBvhId);
        Hell::Bvh::DestroySceneBvh(g_houseLightOccluderSceneBvhId);
        g_houseLightOccluderMeshBvhId = 0;
        g_houseLightOccluderSceneBvhId = 0;

        const glm::vec3 boundsMin = glm::vec3(-999.0f);
        const glm::vec3 boundsMax = glm::vec3(999.0f);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        CreateHouseOccluderGeometry(boundsMin, boundsMax, vertices, indices);

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

		// Add any dynamic mesh nodes to the primitive instances vector
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

        // TODO: Remove me
		//for (Door& door : GetDoors()) {
		//	const std::vector<RenderItem>& renderItems = door.GetRenderItems();
		//	for (const RenderItem& renderItem : renderItems) {
		//		CreateObjectInstanceDataFromRenderItem(renderItem, g_dynamicSceneInstances);
		//	}
		//}
		for (PictureFrame& pictureFrame : Unloved::World::GetPictureFrames()) {
			const std::vector<RenderItem>& renderItems = pictureFrame.GetRenderItems();
			for (const RenderItem& renderItem : renderItems) {
				CreateObjectInstanceDataFromRenderItem(renderItem, g_dynamicSceneInstances);
			}
		}
		for (PickUp& pickUp : Unloved::World::GetPickUps()) {
			const std::vector<RenderItem>& renderItems = pickUp.GetRenderItems();
			for (const RenderItem& renderItem : renderItems) {
				CreateObjectInstanceDataFromRenderItem(renderItem, g_dynamicSceneInstances);
			}
		}

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
        for (ChristmasLightSet& object : Unloved::World::GetChristmasLightSets())	CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Fence& object : Unloved::World::GetFences())							CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Ladder& object : Unloved::World::GetLadders())							CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Mermaid& object : Unloved::World::GetMermaids())						CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (PowerPoleSet& object : Unloved::World::GetPowerPoleSets())			    CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);
        for (Staircase& object : Unloved::World::GetStaircases())					CreateObjectInstanceDataFromRenderItems(object.GetRenderItems(), g_staticSceneInstances);

        // Add any static mesh nodes to the primitive instances vector
        for (Door& object : Unloved::World::GetDoors())                     CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Fireplace& object : Unloved::World::GetFireplaces())           CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (GenericObject& object : Unloved::World::GetGenericObjects())   CreateStaticPrimtiveInstances(object.GetMeshNodes());
        //for (Ladder& object : GetLadders())                 CreateStaticPrimtiveInstances(object.GetMeshNodes()); why didn't this work? some bug
        for (Light& object : Unloved::World::GetLights())					CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Piano& object : Unloved::World::GetPianos())                   CreateStaticPrimtiveInstances(object.GetMeshNodes());
        for (Window& object : Unloved::World::GetWindows())                 CreateStaticPrimtiveInstances(object.GetMeshNodes());

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

    BvhRayResult ClosestHouseLightOccluderHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
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
