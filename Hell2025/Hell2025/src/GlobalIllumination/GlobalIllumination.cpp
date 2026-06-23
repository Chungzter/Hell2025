/*#include "GlobalIllumination.h"
#include "AssetManagement/AssetManager.h"
#include "Bvh/Gpu/BvhOLD.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "Physics/Physics.h"
#include "World/World.h"

#include "Core/Game.h"
#include "Renderer/Renderer.h"

#include "Hell/Logging.h"

namespace GlobalIllumination {

    float g_probeSpacing = 0.75f;
    float g_pointCloudSpacing = 0.4;

    void CreatePointCloud();
    void CreateDoorBvh();
    void CreateHouseBvh();
    void UpdateSceneBVh();

    void InitPointGrid(); // Rename me and put me somewhere better

    inline float RoundUp(float value, float spacing) { return std::ceil(value / spacing) * spacing; }
    inline float RoundDown(float value, float spacing) { return std::floor(value / spacing) * spacing; }

    uint64_t g_houseBvhId = 0;
    uint64_t g_doorBvhId = 0;
    uint64_t g_sceneBvhId = 0;

    glm::vec3 g_houseMinBounds = glm::vec3(0.0f);
    glm::vec3 g_houseMaxBounds = glm::vec3(0.0f);

    std::vector<DDGIVolume> g_lightVolumes;
	std::vector<Triangle> g_triangles;
	std::vector<CloudPoint> g_pointCloud;
	std::vector<CloudPointTextureInfo> g_pointCloudTextureInfo;

    bool g_globalIlluminationStructuresDirty = false;
    bool g_pointCloudNeedsGpuUpdate = false;

    std::vector<PointCloudOctrant> g_pointCloudOctrants;
    std::vector<unsigned int> g_pointIndices;
    constexpr float g_pointCloudOctrantSpacing = 5.0f;
    glm::uvec3 g_pointCloudGridDimensions;
    glm::vec3 g_pointGridWorldMin;
    glm::vec3 g_pointGridWorldMax;
    glm::vec3 g_pointCloudOctrantSize;

    void Update() {
        if (g_globalIlluminationStructuresDirty) {
            GlobalIllumination::CreatePointCloud();
            GlobalIllumination::CreateHouseBvh();

            if (g_doorBvhId == 0) {
                CreateDoorBvh();
            }

            if (g_sceneBvhId == 0) {
                g_sceneBvhId = Bvh::Gpu::CreateNewSceneBvh();
            }

            Bvh::Gpu::FlatternMeshBvhNodes();
            g_globalIlluminationStructuresDirty = false;

            Logging::Debug() << "GlobalIllumination::Update() detected g_globalIlluminationStructuresDirty as true, so recreated point cloud and house bvh\n";
        }

        UpdateSceneBVh();
    }

    void CreatePointCloud() {
        g_triangles.clear();

        // Store floor and ceilings triangles
        for (HousePlane& plane : World::GetHousePlanes()) {
            for (uint32_t i = 0; i < plane.GetIndices().size(); i += 3) {
                Triangle& triangle = g_triangles.emplace_back();

                int idx0 = plane.GetIndices()[i + 0];
                int idx1 = plane.GetIndices()[i + 1];
                int idx2 = plane.GetIndices()[i + 2];

                triangle.v0 = plane.GetVertices()[idx0].position;
                triangle.v1 = plane.GetVertices()[idx1].position;
                triangle.v2 = plane.GetVertices()[idx2].position;

                triangle.uv0 = plane.GetVertices()[idx0].uv;
                triangle.uv1 = plane.GetVertices()[idx1].uv;
                triangle.uv2 = plane.GetVertices()[idx2].uv;

                triangle.normal = plane.GetVertices()[i].normal;

				triangle.baseColorTextureIndex = plane.GetMaterial()->m_basecolor;
				triangle.rmaTextureIndex = plane.GetMaterial()->m_rma;
            }
        }

        // Store wall triangles
        for (Wall& wall : World::GetWalls()) {

            // Skip exterior walls
            if (wall.IsWeatherBoards()) continue;

            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                for (uint32_t i = 0; i < wallSegment.GetIndices().size(); i += 3) {
                    Triangle& triangle = g_triangles.emplace_back();

                    int idx0 = wallSegment.GetIndices()[i + 0];
                    int idx1 = wallSegment.GetIndices()[i + 1];
                    int idx2 = wallSegment.GetIndices()[i + 2];

                    triangle.v0 = wallSegment.GetVertices()[idx0].position;
                    triangle.v1 = wallSegment.GetVertices()[idx1].position;
                    triangle.v2 = wallSegment.GetVertices()[idx2].position;

                    triangle.uv0 = wallSegment.GetVertices()[idx0].uv;
                    triangle.uv1 = wallSegment.GetVertices()[idx1].uv;
                    triangle.uv2 = wallSegment.GetVertices()[idx2].uv;

                    triangle.normal = normalize(
                        wallSegment.GetVertices()[idx0].normal +
                        wallSegment.GetVertices()[idx1].normal +
                        wallSegment.GetVertices()[idx2].normal
					);

					triangle.baseColorTextureIndex = wall.GetMaterial()->m_basecolor;
					triangle.rmaTextureIndex = wall.GetMaterial()->m_rma;
                }
            }
        }

		g_pointCloud.clear();
		g_pointCloudTextureInfo.clear();

        for (Triangle& triangle : g_triangles) {
            // Compute normal
            glm::vec3 edge1 = triangle.v1 - triangle.v0;
            glm::vec3 edge2 = triangle.v2 - triangle.v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            triangle.normal = normal;

            // Choose the up vector based on the normal
            glm::vec3 up = (std::abs(normal.z) > 0.999f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

            // Calculate right and up vectors
            glm::vec3 right = glm::normalize(glm::cross(up, normal));
            up = glm::cross(normal, right);

            glm::mat3 transform(right.x, right.y, right.z,up.x, up.y, up.z, normal.x, normal.y, normal.z);

            glm::vec2 v0_2d(glm::dot(right, triangle.v0), glm::dot(up, triangle.v0));
            glm::vec2 v1_2d(glm::dot(right, triangle.v1), glm::dot(up, triangle.v1));
            glm::vec2 v2_2d(glm::dot(right, triangle.v2), glm::dot(up, triangle.v2));

            // Determine the bounding box of the 2D triangle
            glm::vec2 min = glm::min(glm::min(v0_2d, v1_2d), v2_2d);
            glm::vec2 max = glm::max(glm::max(v0_2d, v1_2d), v2_2d);

            // Round min and max values
            min.x = RoundDown(min.x, g_pointCloudSpacing) - g_pointCloudSpacing * 0.5f;
            min.y = RoundDown(min.y, g_pointCloudSpacing) - g_pointCloudSpacing * 0.5f;
            max.x = RoundUp(max.x, g_pointCloudSpacing) + g_pointCloudSpacing * 0.5f;
            max.y = RoundUp(max.y, g_pointCloudSpacing) + g_pointCloudSpacing * 0.5f;

            float theshold = 0.05f;
            min.x += theshold;
            min.y += theshold;
            max.x -= theshold;
            max.y -= theshold;

            // Generate points within the bounding box
            for (float x = min.x; x <= max.x; x += g_pointCloudSpacing) {
                for (float y = min.y; y <= max.y; y += g_pointCloudSpacing) {
                    glm::vec2 pt(x, y);
                    if (Util::IsPointInTriangle2D(pt, v0_2d, v1_2d, v2_2d)) {
                        glm::vec3 pt3d = triangle.v0 + right * (pt.x - v0_2d.x) + up * (pt.y - v0_2d.y);

                        CloudPoint& cloudPoint = g_pointCloud.emplace_back();
                        cloudPoint.position = glm::vec4(pt3d, 0.0f);
                        cloudPoint.normal = glm::vec4(triangle.normal, 0.0f);

                        // Calculate uv via barycentrics
                        glm::vec3 bary = Util::GetBarycentric(pt, v0_2d, v1_2d, v2_2d);
                        glm::vec2 uv = bary.x * triangle.uv0 + bary.y * triangle.uv1 + bary.z * triangle.uv2;

						CloudPointTextureInfo& cloudPointTextureInfo = g_pointCloudTextureInfo.emplace_back();
						cloudPointTextureInfo.u = uv.x;
						cloudPointTextureInfo.v = uv.y;
						cloudPointTextureInfo.baseColorIndex = triangle.baseColorTextureIndex;
						cloudPointTextureInfo.rmaIndex = triangle.rmaTextureIndex;
                    }
                }
            }
        }
        g_pointCloudNeedsGpuUpdate = true;

        Logging::Debug() << "Recreated point cloud: " << g_pointCloud.size() << " points \n";
        Logging::Debug() << "g_triangles.size():             " << g_triangles.size() << "\n";
        Logging::Debug() << "g_pointCloud.size():            " << g_pointCloud.size() << "\n";
        Logging::Debug() << "g_pointCloudTextureInfo.size(): " << g_pointCloudTextureInfo.size() << "\n";
    }

    void CreateHouseBvh() {
        // Destroy any previous house bvh
        if (g_houseBvhId != 0) {
            Bvh::Gpu::DestroyMeshBvh(g_houseBvhId);
        }

        g_houseMinBounds = glm::vec3(std::numeric_limits<float>::max());
        g_houseMaxBounds = glm::vec3(-std::numeric_limits<float>::max());

        // Create house vertices
        std::vector<Vertex> vertices;
        for (Triangle& triangle : g_triangles) {
            Vertex v0, v1, v2;
            v0.position = triangle.v0;
            v1.position = triangle.v1;
            v2.position = triangle.v2;
            v0.normal = triangle.normal;
            v1.normal = triangle.normal;
            v2.normal = triangle.normal;
            vertices.push_back(triangle.v0);
            vertices.push_back(triangle.v1);
            vertices.push_back(triangle.v2);

            g_houseMinBounds.x = std::min(g_houseMinBounds.x, v0.position.x);
            g_houseMinBounds.y = std::min(g_houseMinBounds.y, v0.position.y);
            g_houseMinBounds.z = std::min(g_houseMinBounds.z, v0.position.z);
            g_houseMaxBounds.x = std::max(g_houseMaxBounds.x, v0.position.x);
            g_houseMaxBounds.y = std::max(g_houseMaxBounds.y, v0.position.y);
            g_houseMaxBounds.z = std::max(g_houseMaxBounds.z, v0.position.z);

            g_houseMinBounds.x = std::min(g_houseMinBounds.x, v1.position.x);
            g_houseMinBounds.y = std::min(g_houseMinBounds.y, v1.position.y);
            g_houseMinBounds.z = std::min(g_houseMinBounds.z, v1.position.z);
            g_houseMaxBounds.x = std::max(g_houseMaxBounds.x, v1.position.x);
            g_houseMaxBounds.y = std::max(g_houseMaxBounds.y, v1.position.y);
            g_houseMaxBounds.z = std::max(g_houseMaxBounds.z, v1.position.z);

            g_houseMinBounds.x = std::min(g_houseMinBounds.x, v2.position.x);
            g_houseMinBounds.y = std::min(g_houseMinBounds.y, v2.position.y);
            g_houseMinBounds.z = std::min(g_houseMinBounds.z, v2.position.z);
            g_houseMaxBounds.x = std::max(g_houseMaxBounds.x, v2.position.x);
            g_houseMaxBounds.y = std::max(g_houseMaxBounds.y, v2.position.y);
            g_houseMaxBounds.z = std::max(g_houseMaxBounds.z, v2.position.z);
        }

        // Create house indices
        std::vector<uint32_t> indices(vertices.size());
        for (int i = 0; i < vertices.size(); i++) {
            indices[i] = i;
        }

        g_houseBvhId = Bvh::Gpu::CreateMeshBvhFromVertexData(vertices, indices);

        g_lightVolumes.clear();

        DDGIVolume& lightVolume = g_lightVolumes.emplace_back();
        lightVolume.Init(g_houseMinBounds, g_houseMaxBounds, GetProbeSpacing());
        lightVolume.GetCreateInfo().saveToFile = false;

        InitPointGrid();
    }

    void CreateDoorBvh() {
        float w = DOOR_DEPTH;
        float h = DOOR_HEIGHT;
        float d = DOOR_WIDTH;

        std::vector<Vertex> vertices;
        vertices.reserve(24);

        // front face
        vertices.emplace_back(Vertex(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, 0), glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, 0), glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(glm::vec3(0, h, 0), glm::vec3(0, 0, 1)));

        // back face
        vertices.emplace_back(Vertex(glm::vec3(0, 0, -d), glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, -d), glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, -d), glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(glm::vec3(0, h, -d), glm::vec3(0, 0, -1)));

        // left face
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, 0), glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, -d), glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, -d), glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, 0), glm::vec3(-1, 0, 0)));

        // right face
        vertices.emplace_back(Vertex(glm::vec3(0, 0, -d), glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(0, h, 0), glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(glm::vec3(0, h, -d), glm::vec3(1, 0, 0)));

        // top face
        vertices.emplace_back(Vertex(glm::vec3(0, h, 0), glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, 0), glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, h, -d), glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(0, h, -d), glm::vec3(0, 1, 0)));

        // bottom face
        vertices.emplace_back(Vertex(glm::vec3(0, 0, -d), glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, -d), glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(-w, 0, 0), glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)));

        std::vector<uint32_t> indices;
        indices.reserve(36);

        // map indices
        for (uint32_t i = 0; i < 6; ++i) {
            uint32_t offset = i * 4;
            indices.push_back(offset + 0);
            indices.push_back(offset + 1);
            indices.push_back(offset + 2);
            indices.push_back(offset + 2);
            indices.push_back(offset + 3);
            indices.push_back(offset + 0);
        }

        g_doorBvhId = Bvh::Gpu::CreateMeshBvhFromVertexData(vertices, indices);
    }

    void UpdateSceneBVh() {
        std::vector<PrimitiveInstance> instances;

        // Add the house
        PrimitiveInstance& instance = instances.emplace_back();
        instance.worldAabbBoundsMin = Bvh::Gpu::GetMeshBvhRootNodeBoundsMin(g_houseBvhId);
        instance.worldAabbBoundsMax = Bvh::Gpu::GetMeshBvhRootNodeBoundsMax(g_houseBvhId);
        instance.objectId = 0;
        instance.worldTransform = glm::mat4(1.0f);
        instance.inverseWorldTransform = glm::inverse(instance.worldTransform);
        instance.meshBvhId = g_houseBvhId;
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
            instance.meshBvhId = g_doorBvhId;
            instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;
        }

        Bvh::Gpu::UpdateSceneBvh(g_sceneBvhId, instances);
    }

    void InitPointGrid() {

        for (DDGIVolume& lightVolume : g_lightVolumes) {

            g_pointGridWorldMin = lightVolume.GetOrigin();
            g_pointGridWorldMax = lightVolume.GetOrigin() + glm::vec3(lightVolume.GetWorldSpaceWidth(), lightVolume.GetWorldSpaceHeight(), lightVolume.GetWorldSpaceDepth());

            glm::vec3 worldSize = g_pointGridWorldMax - g_pointGridWorldMin;
            g_pointCloudGridDimensions.x = static_cast<unsigned int>(glm::ceil(worldSize.x / g_pointCloudOctrantSpacing));
            g_pointCloudGridDimensions.y = static_cast<unsigned int>(glm::ceil(worldSize.y / g_pointCloudOctrantSpacing));
            g_pointCloudGridDimensions.z = static_cast<unsigned int>(glm::ceil(worldSize.z / g_pointCloudOctrantSpacing));

            g_pointCloudOctrantSize = worldSize / glm::vec3(g_pointCloudGridDimensions);

             // Bail if point cloud is empty
            if (g_pointCloud.empty()) return;

            // For each grid cell, count how many points fall inside it
            unsigned int totalCells = g_pointCloudGridDimensions.x * g_pointCloudGridDimensions.y * g_pointCloudGridDimensions.z;
            std::vector<unsigned int> cellCounts(totalCells, 0);

            for (const auto& point : g_pointCloud) {
                // Find out which grid cell this point belongs to
                glm::vec3 relativePos = glm::vec3(point.position) - g_pointGridWorldMin;
                glm::ivec3 cellCoords = glm::ivec3(relativePos / g_pointCloudOctrantSize);

                // Make sure the coordinates are within the grid bounds, just in case
                cellCoords = glm::clamp(cellCoords, glm::ivec3(0), glm::ivec3(g_pointCloudGridDimensions) - 1);

                // Convert the 3D cell coordinate into a 1D array index and increment the counter
                unsigned int cellIndex = (cellCoords.z * g_pointCloudGridDimensions.x * g_pointCloudGridDimensions.y) + (cellCoords.y * g_pointCloudGridDimensions.x) + cellCoords.x;
                cellCounts[cellIndex]++;
            }

            // Create the final PointGridCell structures with the correct offsets
            g_pointCloudOctrants.resize(totalCells);
            unsigned int currentOffset = 0;
            for (unsigned int i = 0; i < totalCells; ++i) {
                g_pointCloudOctrants[i].m_cloudPointCount = cellCounts[i];
                g_pointCloudOctrants[i].m_offset = currentOffset;
                currentOffset += cellCounts[i]; // The next cell's offset starts after all of this cell's points
            }

            // Finally, we create the master list of sorted point indices
            g_pointIndices.resize(g_pointCloud.size());
            std::vector<unsigned int> tempOffsets(totalCells);
            for (unsigned int i = 0; i < totalCells; ++i) {
                tempOffsets[i] = g_pointCloudOctrants[i].m_offset;
            }

            // Go through the original points again...
            for (unsigned int i = 0; i < g_pointCloud.size(); ++i) {
                const auto& point = g_pointCloud[i];

                // Find which cell it belongs to...
                glm::vec3 relativePos = glm::vec3(point.position) - g_pointGridWorldMin;
                glm::ivec3 cellCoords = glm::ivec3(relativePos / g_pointCloudOctrantSize);
                cellCoords = glm::clamp(cellCoords, glm::ivec3(0), glm::ivec3(g_pointCloudGridDimensions) - 1);
                unsigned int cellIndex = (cellCoords.z * g_pointCloudGridDimensions.x * g_pointCloudGridDimensions.y) + (cellCoords.y * g_pointCloudGridDimensions.x) + cellCoords.x;

                // Use the write counter to place the point's original index i in the correct slot
                unsigned int& insertionIndex = tempOffsets[cellIndex];
                g_pointIndices[insertionIndex] = i;

                // Increment the write counter for that cell
                insertionIndex++;
            }
        }
        std::cout << "Point cloud octrant grid created\n";
        std::cout << "g_pointIndices:       " << g_pointIndices.size() << "\n";
        std::cout << "g_pointCloudOctrants: " << g_pointCloudOctrants.size() << "\n";

    }

    uint64_t GetSceneBvhId() {
        return g_sceneBvhId;
    }

    const std::vector<BvhNode>& GetSceneNodes() {
        static std::vector<BvhNode> empty;
        if (g_sceneBvhId == 0) {
            return empty;
        }

        SceneBvh* sceneBvh = Bvh::Gpu::GetSceneBvhById(g_sceneBvhId);
        if (!sceneBvh) return empty;

        return sceneBvh->m_nodes;
    }

    std::vector<CloudPoint>& GetPointClound() {
        return g_pointCloud;
    }

    std::vector<CloudPointTextureInfo>& GetPointCloudTextureInfo() {
        return g_pointCloudTextureInfo;
    }

	//std::vector<LightVolume>& GetLightVolumes() {
	//    return g_lightVolumes;
	//}

    DDGIVolume& GetTestLightVolume() {
        static DDGIVolume invalid;

		//if (g_lightVolumes.size() == 1) {
		//	return g_lightVolumes[0];
		//}
		//if (g_lightVolumes.size() > 1) {
        //    Logging::Fatal() << "LightVolume& GetTestLightVolumes() fucked up, you have more than one LightVolume and ALL your code assumes you only have one\n";
		//	return invalid;
		//}
		//else {
		//	Logging::Fatal() << "LightVolume& GetTestLightVolumes() fucked up, you have zero LightVolumes and ALL your code assumes you only have one\n";
		//	return invalid;
        //}

        
        Hell::SlotMap<DDGIVolume>& ddgiVolumes = World::GetDDGIVolumes();

        if (ddgiVolumes.size() > 1) {
            Logging::Fatal() << "LightVolume& GetTestLightVolumes() fucked up, you have more than one LightVolume and ALL your code assumes you only have one\n";
            return invalid;
        }
        if (ddgiVolumes.size() == 1) {
            for (DDGIVolume& ddgiVolume : ddgiVolumes) {
                return ddgiVolume;
            }
        }
        else {
            Logging::Fatal() << "LightVolume& GetTestLightVolumes() fucked up, you have zero LightVolumes and ALL your code assumes you only have one\n";
            return invalid;
        }
    }

    void SetGlobalIlluminationStructuresDirtyState(bool state) {
        g_globalIlluminationStructuresDirty = state;
    }

    bool GlobalIlluminationStructuresAreDirty() {
        return g_globalIlluminationStructuresDirty;
    }

    void SetPointCloudNeedsGpuUpdateState(bool state) {
        g_pointCloudNeedsGpuUpdate = state;
    }

    bool PointCloudNeedsGpuUpdate() {
        return g_pointCloudNeedsGpuUpdate;
    }

    float GetProbeSpacing() {
        return g_probeSpacing;
    }

    float GetPointCloudSpacing() {
        return g_pointCloudSpacing;
    }

    std::vector<PointCloudOctrant>& GetPointCloudOctrants() {
        return g_pointCloudOctrants;
    }

    std::vector<unsigned int>& GetPointIndices() {
        return g_pointIndices;
    }

    glm::uvec3 GetPointCloudGridDimensions() {
        return g_pointCloudGridDimensions;
    }

    glm::vec3 GetPointGridWorldMin() {
        return g_pointGridWorldMin;
    }

    glm::vec3 GetPointGridWorldMax() {
        return g_pointGridWorldMax;
    }

    std::vector<DDGIVolumeCreateInfo> GetDDGIVolumesCreateInfo() {
        std::vector<DDGIVolumeCreateInfo> createInfos;
        
        for (DDGIVolume& volume : g_lightVolumes) {
            createInfos.push_back(volume.GetCreateInfo());
        }

        return createInfos;
    }
}

*/
