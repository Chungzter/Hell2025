#include "PlanarQuadObject.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Geometry/Geometry.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/House/HouseGeometryBuilder.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/mat3x3.hpp>

#include <cmath>

namespace Unloved {

    void PlanarQuadObject::Rebuild() {
        Reset();

        switch (m_createInfo.type) {
        case PlanarQuadObjectType::DECKING_BOARDS: RebuildDeckingBoards(); break;
        case PlanarQuadObjectType::GUTTER:         RebuildGutter(); break;
        default: break;
        }

        WorldBVH::MarkStaticSceneBvhDirty();
    }

    void PlanarQuadObject::RebuildDeckingBoards() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetDeckingBoardsSourceMesh();
        std::vector<Vertex> vertices = sourceMesh.vertices;
        std::vector<uint32_t> indices = sourceMesh.indices;

        const glm::mat4& worldMatrix = m_planarQuad.GetWorldMatrixP1();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);
        const float width = m_planarQuad.GetWidth();
        const float depth = m_planarQuad.GetDepth();
        const float uvScale = m_createInfo.customFloats[0];
        const bool rotateUVs = m_createInfo.customBools[0];

        for (Vertex& vertex : vertices) {
            if (std::abs(vertex.position.x) > 0.01f) vertex.position.x = width;
            if (std::abs(vertex.position.z) > 0.01f) vertex.position.z = -depth;

            // if (std::abs(vertex.normal.y) > 0.5f) {
            //     vertex.uv.x *= width;
            //     vertex.uv.y *= depth;
            // }
            // else if (std::abs(vertex.normal.x) > 0.5f) vertex.uv.x *= depth;
            // else if (std::abs(vertex.normal.z) > 0.5f) vertex.uv.x *= width;

            vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

            vertex.uv *= uvScale;

            if (rotateUVs) {
                const glm::vec2 uv = vertex.uv;
                vertex.uv = glm::vec2(uv.y, -uv.x);
            }

            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
        }

        // Create mesh
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "DeckingBoards");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;

        // Create BVH
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        // Create render item
        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialNames[0]);
        const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
        m_renderItems.push_back(renderItem);

        // Create physics object
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);
    }

    void PlanarQuadObject::RebuildGutter() {
        // Gutter
        {
            const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterSourceMesh();
            std::vector<Vertex> vertices = sourceMesh.vertices;
            std::vector<uint32_t> indices = sourceMesh.indices;

            const glm::mat4& worldMatrix = m_planarQuad.GetWorldMatrixP1();
            const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);
            const float width = m_planarQuad.GetWidth();
            const float depth = m_planarQuad.GetDepth();
            const float uvScale = m_createInfo.customFloats[0];
            const bool rotateUVs = m_createInfo.customBools[0];

            for (Vertex& vertex : vertices) {
                if (std::abs(vertex.position.x) > 0.5f) {
                    vertex.position.x = width;
                    vertex.uv.x = vertex.position.x;
                }

                vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            // Create mesh
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;

            // Create BVH
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            // Create render item
            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Brass");
            const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);
        }

        // Awning
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            HouseGeometryBuilder::CreateDownFacingPlane(m_planarQuad, vertices, indices);

            // Mesh
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;

            // Create BVH
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            // Create render item
            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Brass");
            const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);
        }
    }
}
