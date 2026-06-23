#include "BvhOLD.h"
#include "Debug/DebugDraw.h"

namespace Bvh::Gpu {

    void RenderMesh(uint64_t bvhId, glm::vec4 color, glm::mat4 worldTransform) {
        if (!MeshBvhExists(bvhId)) return;
            
        MeshBvh* meshBvh = GetMeshBvhById(bvhId);
        for (const BVHTriangle& triangle : meshBvh->m_triangles) {
            glm::vec3 p0 = glm::vec3(triangle.v0_and_e1x);
            glm::vec3 e1 = glm::vec3(triangle.v0_and_e1x.w, triangle.e1yz_and_e2xy.x, triangle.e1yz_and_e2xy.y);
            glm::vec3 e2 = glm::vec3(triangle.e1yz_and_e2xy.z, triangle.e1yz_and_e2xy.w, triangle.e2z_and_normal.x);
            glm::vec3 p1 = p0 - e1;
            glm::vec3 p2 = p0 + e2;
            p0 = worldTransform * glm::vec4(p0, 1.0f);
            p1 = worldTransform * glm::vec4(p1, 1.0f);
            p2 = worldTransform * glm::vec4(p2, 1.0f);
            DebugDraw::DrawLine(p0, p1, color);
            DebugDraw::DrawLine(p1, p2, color);
            DebugDraw::DrawLine(p2, p0, color);
        }
    }

    void RenderSceneBvh(uint64_t bvhId, glm::vec4 color) {
        if (!SceneBvhExists(bvhId)) return;

        SceneBvh* sceneMeshBvh = GetSceneBvhById(bvhId);
        glm::mat4 worldTransform = glm::mat4(1.0f);


        //std::cout << "node count: " << sceneMeshBvh->m_nodes.size() << "\n";

        for (BvhNode& node : sceneMeshBvh->m_nodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DebugDraw::DrawAABB(aabb, color, worldTransform);

            //]std::cout << "- AABB min: " << aabb.GetBoundsMin();
            //]std::cout << "- AABB max: " << aabb.GetBoundsMax();
        }
       // std::cout << "\n";
    }

    void RenderMeshBvh(uint64_t bvhId, glm::vec4 color, glm::mat4 worldTransform) {
        if (!MeshBvhExists(bvhId)) return;

        MeshBvh* meshBvh = GetMeshBvhById(bvhId);

        for (BvhNode& node : meshBvh->m_nodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DebugDraw::DrawAABB(aabb, color, worldTransform);
        }
    }
}
