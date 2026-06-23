#include "DDGIVolume.h"

#include "Debug/DebugDraw.h"
#include "AssetManagement/AssetManager.h" // remove me
#include "World/World.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Logging.h"

DDGIVolume::DDGIVolume(uint64_t id, DDGIVolumeCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_id = id;
    m_createInfo = createInfo;
    m_createInfo.origin += spawnOffset.translation; 
    m_createInfo.rotation += glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    UpdateMembers();

    std::cout << "Total probe count: " << GetTotalProbeCount() << "\n";
}

void DDGIVolume::Update() {
    if (m_raytracingDataDirty) {
        CreateRaytracingData();
        m_raytracingDataDirty = false;
    }

    //CreateDoorBvh();
    // Also in here, find a way to do an Immediate style upload of the point cloud data + compute point light base color
    // This will be handy for Vulkan also.

    m_pointCloud.Update();
}

void DDGIVolume::CleanUp() {
    CleanUpRaytracingData();
}

void DDGIVolume::CleanUpRaytracingData() {
    Hell::Bvh::DestroyMeshBvh(m_doorBvhId);
    Hell::Bvh::DestroyMeshBvh(m_houseBvhId);
    Hell::Bvh::DestroySceneBvh(m_sceneBvhId);

    m_doorBvhId = 0;
    m_houseBvhId = 0;
    m_sceneBvhId = 0;
    m_probePointIndexPoolSize = 0;

    m_triangles.clear();
    m_pointCloud.CleanUp();

}

void DDGIVolume::CreateRaytracingData() {
    CleanUpRaytracingData();

    CreateTriangleData();
    CreateHouseBvh();
    CreateDoorBvh(); // Probably rewrite this so doors internally manager their own BVH, that way stained glass ones can have holes

    m_sceneBvhId = Hell::Bvh::CreateSceneBvh();
    if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId)) {
        std::vector<SceneBvhMeshInput> meshBvhs;
        if (MeshBvh* houseMeshBvh = Hell::Bvh::GetMeshBvhById(m_houseBvhId)) {
            meshBvhs.push_back({ m_houseBvhId, houseMeshBvh });
        }
        if (MeshBvh* doorMeshBvh = Hell::Bvh::GetMeshBvhById(m_doorBvhId)) {
            meshBvhs.push_back({ m_doorBvhId, doorMeshBvh });
        }
        sceneBvh->AddMeshBvhs(meshBvhs);
    }

    CreatePointCloud();
    CalculateProbePointIndexPoolSize();
}

void DDGIVolume::CreateTriangleData() {
    m_triangles.clear();
    std::vector<HouseOccluderTriangle> houseOccluderTriangles;
    World::CreateHouseOccluderTriangles(m_boundsMin, m_boundsMax, houseOccluderTriangles);
    m_triangles.reserve(houseOccluderTriangles.size());

    for (const HouseOccluderTriangle& sourceTriangle : houseOccluderTriangles) {
        Triangle& triangle = m_triangles.emplace_back();
        triangle.v0 = sourceTriangle.v0;
        triangle.v1 = sourceTriangle.v1;
        triangle.v2 = sourceTriangle.v2;
        triangle.uv0 = sourceTriangle.uv0;
        triangle.uv1 = sourceTriangle.uv1;
        triangle.uv2 = sourceTriangle.uv2;
        triangle.normal = sourceTriangle.normal;
        triangle.baseColorTextureIndex = sourceTriangle.baseColorTextureIndex;
        triangle.rmaTextureIndex = sourceTriangle.rmaTextureIndex;
    }

    // Debug
    int i = 0;
    std::vector<Vertex> vertices;
    vertices.reserve(m_triangles.size() * 3);
    std::vector<uint32_t> indices;
    indices.reserve(m_triangles.size() * 3);
    for (Triangle& triangle : m_triangles) {
        Vertex& v0 = vertices.emplace_back();
        v0.position = triangle.v0;
        v0.normal = triangle.normal;
        indices.push_back(i++);

        Vertex& v1 = vertices.emplace_back();
        v1.position = triangle.v1;
        v1.normal = triangle.normal;
        indices.push_back(i++);

        Vertex& v2 = vertices.emplace_back();
        v2.position = triangle.v2;
        v2.normal = triangle.normal;
        indices.push_back(i++);
    }
    m_staticMeshBuffer.AddMesh(vertices, indices, "FUCK");
    m_staticMeshBuffer.UpdateBuffers();
}

void DDGIVolume::CreateHouseBvh() {
    if (m_houseBvhId != 0) {
        Hell::Bvh::DestroyMeshBvh(m_houseBvhId);
        m_houseBvhId = 0;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    World::CreateHouseOccluderGeometry(m_boundsMin, m_boundsMax, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    m_houseBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
}

void DDGIVolume::CreateDoorBvh() { 
    // ATTENTION:
    // Probably rewrite this so doors internally manager their own BVH, that way stained glass ones can have holes
    // You'll need a function that gets PxBox vertices from PxShape box and computes normals, that'd do it.

    float w = DOOR_DEPTH;
    float h = DOOR_HEIGHT;
    float d = DOOR_WIDTH;

    std::vector<Vertex> vertices;
    vertices.reserve(24);

    float paddingPosX = 0.01f;
    float paddingPosY = 0.03f;
    float paddingPosZ = 0.02f;
    float paddingNegX = 0.08f;
    float paddingNegY = 0.03f;
    float paddingNegZ = 0.02f;

    // Corners
    glm::vec3 p0 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom right
    glm::vec3 p1 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom left
    glm::vec3 p2 = glm::vec3(-w - paddingNegX, h + paddingPosY, 0 + paddingPosZ); // front top left
    glm::vec3 p3 = glm::vec3(0 + paddingPosX, h + paddingPosY, 0 + paddingPosZ); // front top right
    glm::vec3 p4 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, -d - paddingNegZ); // back bottom right
    glm::vec3 p5 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, -d - paddingNegZ); // back bottom left
    glm::vec3 p6 = glm::vec3(-w - paddingNegX, h + paddingPosY, -d - paddingNegZ); // back top left
    glm::vec3 p7 = glm::vec3(0 + paddingPosX, h + paddingPosY, -d - paddingNegZ); // back top right

    // front face
    vertices.emplace_back(Vertex(p0, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p1, glm::vec3(0, 0, 1)));

    // back face
    vertices.emplace_back(Vertex(p5, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, 0, -1)));

    // left face
    vertices.emplace_back(Vertex(p1, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(-1, 0, 0)));

    // right face
    vertices.emplace_back(Vertex(p4, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p3, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(1, 0, 0)));

    // top face
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 1, 0)));

    // bottom face
    vertices.emplace_back(Vertex(p1, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(0, -1, 0)));

    // Indices
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,       // front
        4, 5, 6, 6, 7, 4,       // back
        8, 9, 10, 10, 11, 8,    // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20  // bottom
    };

    m_doorBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
}

void DDGIVolume::CreatePointCloud() {
    m_pointCloud.Create(m_triangles, GetBoundsMin(), GetBoundsMax(), GetPointCloudSpacing(), 3.0f);
    m_pointCloudNeedsGpuUpload = true;
}

void DDGIVolume::CalculateProbePointIndexPoolSize() {
    const PointCloud& pointCloud = GetPointClound();
    const glm::ivec3 gridDims = pointCloud.GetGridDimensions();
    const float gridCellSize = pointCloud.GetGridCellSize();
    const std::vector<uint32_t>& gridCellCounts = pointCloud.GetGridCellCounts();

    // Calculate how many probes originate in a single point-grid cell
    float probesPerAxis = gridCellSize / GetProbeSpacing();
    uint32_t probesPerCell = static_cast<uint32_t>(std::ceil(probesPerAxis * probesPerAxis * probesPerAxis));

    m_probePointIndexPoolSize = 0;

    // Map wide density scan
    for (int z = 0; z < gridDims.z; ++z) {
        for (int y = 0; y < gridDims.y; ++y) {
            for (int x = 0; x < gridDims.x; ++x) {

                uint32_t pointsIn27Cells = 0;

                // Sum all points in the 3x3x3 neighborhood of this cell
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            int nx = x + dx;
                            int ny = y + dy;
                            int nz = z + dz;

                            if (nx >= 0 && nx < gridDims.x &&
                                ny >= 0 && ny < gridDims.y &&
                                nz >= 0 && nz < gridDims.z) {

                                // Flatten the 3D coords to get the point count for this specific cell
                                int cellIdx = nx + ny * gridDims.x + nz * gridDims.x * gridDims.y;
                                pointsIn27Cells += gridCellCounts[cellIdx];
                            }
                        }
                    }
                }

                // Every probe that could possibly "start" in this cell is allocated the full point-count of its neighborhood
                m_probePointIndexPoolSize += (pointsIn27Cells * probesPerCell);
            }
        }
    }
}

void DDGIVolume::UpdateSceneBvh() {
    std::vector<PrimitiveInstance> instances;

    MeshBvh* houseMeshBvh = Hell::Bvh::GetMeshBvhById(m_houseBvhId);
    if (!houseMeshBvh || houseMeshBvh->m_nodes.empty()) {
        return;
    }

    // Add the house
    PrimitiveInstance& instance = instances.emplace_back();
    instance.worldAabbBoundsMin = houseMeshBvh->m_nodes[0].boundsMin; // This works because the house mesh never rotates
    instance.worldAabbBoundsMax = houseMeshBvh->m_nodes[0].boundsMax; // This works because the house mesh never rotates
    instance.objectId = 0;
    instance.worldTransform = glm::mat4(1.0f);
    instance.inverseWorldTransform = glm::inverse(instance.worldTransform);
    instance.meshBvhId = m_houseBvhId;
    instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;

    // Add all the doors
    for (Door& door : World::GetDoors()) {
        // Bit of a hack but get the hinges matrix, then zero out the y translation to match the doors main model matrix
        MeshNode* meshNode = door.GetMeshNodes().GetMeshNodeByMeshName("Door_Hinges");
        glm::mat4 worldMatrix = meshNode->worldMatrix;
        worldMatrix[3][1] = door.GetDoorModelMatrix()[3][1];

        // The PhysX aabb is tighter than the one your MeshNode holds, so use that
        const AABB& aabb = door.GetPhsyicsAABB();

        PrimitiveInstance& instance = instances.emplace_back();
        instance.worldAabbBoundsMin = aabb.GetBoundsMin();
        instance.worldAabbBoundsMax = aabb.GetBoundsMax();
        instance.objectId = door.GetObjectId();
        instance.worldTransform = worldMatrix;
        instance.inverseWorldTransform = glm::inverse(instance.worldTransform);
        instance.meshBvhId = m_doorBvhId;
        instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;
    }

    SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId);
    if (!sceneBvh) return;

    if (!Hell::Bvh::AddInstanceMeshBvhsToSceneBvh(m_sceneBvhId, instances)) return;
    sceneBvh->UpdateInstances(instances);
}

void DDGIVolume::DebugDraw() {
    // Draw volume bounds as AABB
    if (false) {
        AABB aabb = AABB(GetBoundsMin(), GetBoundsMax());
        DebugDraw::DrawAABB(aabb, YELLOW);
    }

    // Draw probe positions as points
    if (false) {
        for (uint32_t x = 0; x < m_probeCountX; x++) {
            for (uint32_t y = 0; y < m_probeCountY; y++) {
                for (uint32_t z = 0; z < m_probeCountZ; z++) {
                    glm::vec3 probePosition = GetProbeBaseWorldPosition(glm::ivec3(x, y, z));
                    DebugDraw::DrawPoint(probePosition, RED);
                }
            }
        }
    }
}

void DDGIVolume::UpdateMembers() {
    m_boundsMin = m_createInfo.origin - m_createInfo.extents * 0.5f;
    m_boundsMax = m_createInfo.origin + m_createInfo.extents * 0.5f;

    m_worldSpaceWidth = m_boundsMax.x - m_boundsMin.x;
    m_worldSpaceHeight = m_boundsMax.y - m_boundsMin.y;
    m_worldSpaceDepth = m_boundsMax.z - m_boundsMin.z;

    m_probeCountX = (int)std::ceil(m_worldSpaceWidth / m_createInfo.probeSpacing) + 1;
    m_probeCountY = (int)std::ceil(m_worldSpaceHeight / m_createInfo.probeSpacing) + 1;
    m_probeCountZ = (int)std::ceil(m_worldSpaceDepth / m_createInfo.probeSpacing) + 1;

    m_raytracingDataDirty = true;
}

void DDGIVolume::Init(const glm::vec3& aabbMin, const glm::vec3& aabbMax, float probeSpacing) {
    glm::vec3 inflatedAabbMin = aabbMin - glm::vec3(1.0f);
    glm::vec3 inflatedAabbMax = aabbMax + glm::vec3(1.0f);

    m_createInfo.origin = (inflatedAabbMin + inflatedAabbMax) * 0.5f;

    m_worldSpaceWidth = inflatedAabbMax.x - inflatedAabbMin.x;
    m_worldSpaceHeight = inflatedAabbMax.y - inflatedAabbMin.y;
    m_worldSpaceDepth = inflatedAabbMax.z - inflatedAabbMin.z;

    m_createInfo.probeSpacing = probeSpacing;
    m_probeCountX = (int)std::ceil(m_worldSpaceWidth / GetProbeSpacing()) + 1;
    m_probeCountY = (int)std::ceil(m_worldSpaceHeight / GetProbeSpacing()) + 1;
    m_probeCountZ = (int)std::ceil(m_worldSpaceDepth / GetProbeSpacing()) + 1;

    Logging::Debug() << "Created DDGIVolume " << aabbMin << " boundsMin " << aabbMin << " boundsMax\n";
}

uint32_t DDGIVolume::GetTotalProbeCount() const {
    return m_probeCountX * m_probeCountY * m_probeCountZ;
}

DDGIVolumeGPU DDGIVolume::GetGPUData() const {
    glm::vec3 halfExtents = glm::vec3(m_worldSpaceWidth, m_worldSpaceHeight, m_worldSpaceDepth) * 0.5f;

    DDGIVolumeGPU volume;
    volume.origin = GetOrigin();
    volume.probeSpacing = GetProbeSpacing();
    volume.probeCounts = glm::ivec3(m_probeCountX, m_probeCountY, m_probeCountZ);
    volume.numProbes = GetTotalProbeCount(); // sort this out, uint vs int
    volume.worldBoundsMin = GetOrigin() - halfExtents;
    volume.padding0 = 0;
    volume.worldBoundsMax = GetOrigin() + halfExtents;
    volume.padding1 = 0;

    return volume;
}

void DDGIVolume::SetEditorName(const std::string& name) {
    m_createInfo.editorName = name;
}

void DDGIVolume::SetOrigin(const glm::vec3& origin) {
    m_createInfo.origin = origin;
    UpdateMembers();
}

void DDGIVolume::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    UpdateMembers();
}

void DDGIVolume::SetExtents(const glm::vec3& extents) {
    m_createInfo.extents = extents;
    UpdateMembers();
}

void DDGIVolume::SetProbeSpacing(float spacing) {
    m_createInfo.probeSpacing = spacing;
    UpdateMembers();
}

glm::vec3 DDGIVolume::GetProbeBaseWorldPosition(const glm::ivec3& probeCoords) const {
    const glm::vec3 counts = glm::vec3(m_probeCountX, m_probeCountY, m_probeCountZ);
    const glm::vec3 coords = glm::vec3(probeCoords);
    return GetOrigin() + (coords - (counts - 1.0f) * 0.5f) * GetProbeSpacing();
}

const std::vector<BvhNode>& DDGIVolume::GetSceneNodes() {
    static std::vector<BvhNode> empty;
    if (m_sceneBvhId == 0) {
        return empty;
    }

    SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId);
    if (!sceneBvh) return empty;

    return sceneBvh->m_nodes;
}
