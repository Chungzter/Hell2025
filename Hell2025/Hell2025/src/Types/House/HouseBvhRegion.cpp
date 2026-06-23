#include "HouseBvhRegion.h"
#include "AssetManagement/AssetManager.h"
#include "Bvh/Cpu/CpuBvh.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "World/World.h"

void HouseBvhRegion::Update(const glm::vec3& worldBoundsMin, const glm::vec3& worldBoundsMax) {
    m_worldBoundsMin = worldBoundsMin;
    m_worldBoundsMax = worldBoundsMax;
    m_triangles.clear();

    // Gather floor and ceilings triangles
    for (HousePlane& plane : World::GetHousePlanes()) {

        // Skip door floors, they're recreate in the wall gap filler below
        if (plane.GetParentDoorId() != 0) continue;

        for (uint32_t i = 0; i < plane.GetIndices().size(); i += 3) {
            int idx0 = plane.GetIndices()[i + 0];
            int idx1 = plane.GetIndices()[i + 1];
            int idx2 = plane.GetIndices()[i + 2];

            glm::vec3 v0 = plane.GetVertices()[idx0].position;
            glm::vec3 v1 = plane.GetVertices()[idx1].position;
            glm::vec3 v2 = plane.GetVertices()[idx2].position;

            // Get triangle bounds
            glm::vec3 triMin = glm::min(glm::min(v0, v1), v2);
            glm::vec3 triMax = glm::max(glm::max(v0, v1), v2);

            // Cull if triangle is completely outside the exact volume bounds
            if (triMax.x < m_worldBoundsMin.x || triMin.x > m_worldBoundsMax.x ||
                triMax.y < m_worldBoundsMin.y || triMin.y > m_worldBoundsMax.y ||
                triMax.z < m_worldBoundsMin.z || triMin.z > m_worldBoundsMax.z) {
                continue;
            }

            Triangle& triangle = m_triangles.emplace_back();
            triangle.v0 = v0;
            triangle.v1 = v1;
            triangle.v2 = v2;
            triangle.uv0 = plane.GetVertices()[idx0].uv;
            triangle.uv1 = plane.GetVertices()[idx1].uv;
            triangle.uv2 = plane.GetVertices()[idx2].uv;
            triangle.baseColorTextureIndex = plane.GetMaterial()->m_basecolor;
            triangle.rmaTextureIndex = plane.GetMaterial()->m_rma;
        }
    }

    // Gather wall triangles
    for (Wall& wall : World::GetWalls()) {

        // Skip exterior walls
        //if (wall.IsWeatherBoards()) continue;

        for (WallSegment& wallSegment : wall.GetWallSegments()) {
            for (uint32_t i = 0; i < wallSegment.GetIndices().size(); i += 3) {
                int idx0 = wallSegment.GetIndices()[i + 0];
                int idx1 = wallSegment.GetIndices()[i + 1];
                int idx2 = wallSegment.GetIndices()[i + 2];

                glm::vec3 v0 = wallSegment.GetVertices()[idx0].position;
                glm::vec3 v1 = wallSegment.GetVertices()[idx1].position;
                glm::vec3 v2 = wallSegment.GetVertices()[idx2].position;

                // Get triangle bounds
                glm::vec3 triMin = glm::min(glm::min(v0, v1), v2);
                glm::vec3 triMax = glm::max(glm::max(v0, v1), v2);

                // Cull if triangle is completely outside the exact volume bounds
                if (triMax.x < m_worldBoundsMin.x || triMin.x > m_worldBoundsMax.x ||
                    triMax.y < m_worldBoundsMin.y || triMin.y > m_worldBoundsMax.y ||
                    triMax.z < m_worldBoundsMin.z || triMin.z > m_worldBoundsMax.z) {
                    continue;
                }

                Triangle& triangle = m_triangles.emplace_back();
                triangle.v0 = v0;
                triangle.v1 = v1;
                triangle.v2 = v2;
                triangle.uv0 = wallSegment.GetVertices()[idx0].uv;
                triangle.uv1 = wallSegment.GetVertices()[idx1].uv;
                triangle.uv2 = wallSegment.GetVertices()[idx2].uv;
                triangle.baseColorTextureIndex = wall.GetMaterial()->m_basecolor;
                triangle.rmaTextureIndex = wall.GetMaterial()->m_rma;
            }
        }
    }

    // Seal the door gaps in the walls
    for (Door& door : World::GetDoors()) {
        Material* material = Hell::ResourceManager::GetMaterialByName("Ceiling2"); // Rethink this?
        const glm::mat4& modelMatrix = door.GetDoorModelMatrix();

        float padding = 0.02f; // matches clipping cube padding
        float halfP = padding * 0.5f;
        float halfD = DOOR_WIDTH * 0.5f + halfP;
        float h = DOOR_HEIGHT + halfP;
        float halfW = 0.05f; // half of 0.1f wall thickness

        // define 8 corners in local space (origin bottom center)
        glm::vec3 p[8];
        p[0] = glm::vec3(halfW, 0, halfD); // front bottom right
        p[1] = glm::vec3(-halfW, 0, halfD); // front bottom left
        p[2] = glm::vec3(-halfW, h, halfD); // front top left
        p[3] = glm::vec3(halfW, h, halfD); // front top right
        p[4] = glm::vec3(halfW, 0, -halfD); // back bottom right
        p[5] = glm::vec3(-halfW, 0, -halfD); // back bottom left
        p[6] = glm::vec3(-halfW, h, -halfD); // back top left
        p[7] = glm::vec3(halfW, h, -halfD); // back top right

        // transform corners to world space
        for (int i = 0; i < 8; ++i) {
            p[i] = glm::vec3(modelMatrix * glm::vec4(p[i], 1.0f));
        }

        // Helper to add triangles with inverted (CW) winding
        auto addFace = [&](int i0, int i1, int i2, int i3) {
            // triangle 1
            Triangle& t1 = m_triangles.emplace_back();
            t1.v0 = p[i0]; t1.v1 = p[i1]; t1.v2 = p[i2];
            t1.baseColorTextureIndex = material->m_basecolor;
            t1.rmaTextureIndex = material->m_rma;

            // triangle 2
            Triangle& t2 = m_triangles.emplace_back();
            t2.v0 = p[i2]; t2.v1 = p[i3]; t2.v2 = p[i0];
            t2.baseColorTextureIndex = material->m_basecolor;
            t2.rmaTextureIndex = material->m_rma;
            };

        // Build faces with CW winding (points normals inward)
        addFace(0, 1, 2, 3); // front
        addFace(5, 4, 7, 6); // back
        //addFace(1, 5, 6, 2); // left
        //addFace(4, 0, 3, 7); // right
        addFace(3, 2, 6, 7); // top
        addFace(1, 0, 4, 5); // bottom

    }

    // Recompute normals
    for (Triangle& triangle : m_triangles) {
        glm::vec3 edge1 = triangle.v1 - triangle.v0;
        glm::vec3 edge2 = triangle.v2 - triangle.v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        triangle.normal = normal;
    }

    // Destroy any previous house bvh
    if (m_cpuMeshBvhId != 0) {
        Bvh::Cpu::DestroyMeshBvh(m_cpuMeshBvhId);
    }

    // Create house vertices
    std::vector<Vertex> vertices;
    for (Triangle& triangle : m_triangles) {
        Vertex v0, v1, v2;
        v0.position = triangle.v0;
        v1.position = triangle.v1;
        v2.position = triangle.v2;
        v0.normal = triangle.normal;
        v1.normal = triangle.normal;
        v2.normal = triangle.normal;
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
    }

    // Create house indices
    std::vector<uint32_t> indices(vertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        indices[i] = i;
    }

    // From here onwards is CPU specific only. 
    // TODO: Consider restructuring such that this HouseBvhRegion can be used for GPU ray shit also, aka the DDGIVolume stuff 
    // TODO: or better yet, merge both the concepts of all the GPU and CPU bvh shit into a single thing that handles both
    Bvh::Cpu::DestroyMeshBvh(m_cpuMeshBvhId);
    Bvh::Cpu::DestroySceneBvh(m_cpuSceneBvhId);

    m_cpuMeshBvhId = Bvh::Cpu::CreateMeshBvhFromVertexData(vertices, indices);
    Bvh::Cpu::FlatternMeshBvhNodes();

    std::vector<PrimitiveInstance> instances;

    PrimitiveInstance& instance = instances.emplace_back();
    instance.meshBvhId = m_cpuMeshBvhId;
    instance.worldAabbBoundsMin = m_worldBoundsMin;
    instance.worldAabbBoundsMax = m_worldBoundsMax;
    instance.worldTransform = glm::mat4(1.0f);
    instance.inverseWorldTransform = glm::mat4(1.0f);
    instance.worldAabbCenter = (m_worldBoundsMin + m_worldBoundsMax) * 0.5f;

    m_cpuSceneBvhId = Bvh::Cpu::CreateNewSceneBvh();
    Bvh::Cpu::UpdateSceneBvh(m_cpuSceneBvhId, instances);
}

BvhRayResult HouseBvhRegion::CastRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float rayLength) {
    return Bvh::Cpu::ClosestHit(m_cpuSceneBvhId, rayOrigin, rayDir, rayLength);
}

bool HouseBvhRegion::CpuBvhExists() {
    return Bvh::Cpu::SceneBvhExists(m_cpuSceneBvhId) && Bvh::Cpu::MeshBvhExists(m_cpuMeshBvhId);
}
