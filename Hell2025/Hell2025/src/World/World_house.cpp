#include "World.h"
#include "AssetManagement/AssetManager.h"
#include "File/JSON.h"
#include "Managers/HouseManager.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"

namespace World {

    HouseBvhRegion g_houseBvhRegion;

    void RecreateAllHouseGeometry() {
        RecreateAllProceduralWallMesh();
        RecreateAllProcedularHousePlaneMesh();
        RecreateAllWeatherBoards();
        RecreateAllHangingLightCords();
        RecreateAllWallTrims();

        glm::vec3 aabbMin = glm::vec3(-999.0f);
        glm::vec3 aabbMax = glm::vec3(999.0f);
        g_houseBvhRegion.Update(aabbMin, aabbMax);

        for (Light& light : GetLights()) {
            light.RaycastWorldBounds();
        }
    }

    void RecreateAllWallTrims() {
        Hell::SlotMap<TrimSet>& trimSets = GetTrimSets();
        trimSets.clear();

		for (Wall& wall : GetWalls()) {
            if (wall.GetWallType() == WallType::WEATHER_BOARDS) continue;

			const WallCreateInfo& createInfo = wall.GetCreateInfo();

			// Ceiling trim
			TrimSetCreateInfo createInfoCeiling;
			for (const glm::vec3& point : createInfo.points) {
				glm::vec3 trimPoint = point + glm::vec3(0.0f, createInfo.height, 0.0f);
                trimPoint.y -= 0.01f; // safety threshold
				createInfoCeiling.points.push_back(trimPoint);
				createInfoCeiling.type = TrimSetType::CEILING_FANCY;
                createInfoCeiling.trimScale = 0.95f;
			}

			World::AddTrimSet(createInfoCeiling, SpawnOffset());
		}
    }

    void RecreateAllProceduralWallMesh() {
        // Update clipping cubes first, so that CSG is correct
        RecreateClippingCubes();

        for (Wall& wall : GetWalls()) {

            // Update CSG and trims
            wall.UpdateSegmentsTrimsAndVertexData();

            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                // Remove old mesh
                Renderer::RemoveProcedualMeshByMeshId(wallSegment.GetMeshId());

                // Create new mesh
                uint64_t meshId = Renderer::AddProcedualMesh(wallSegment.GetVertices(), wallSegment.GetIndices(), "WallSegment");

                // Update mesh Id
                wallSegment.SetMeshId(meshId);
            }
        }
    }

    void RecreateAllProcedularHousePlaneMesh() {
        for (HousePlane& housePlane : GetHousePlanes()) {
            // Remove old mesh
            Renderer::RemoveProcedualMeshByMeshId(housePlane.GetMeshId());

            // Create new mesh
            uint64_t meshId = Renderer::AddProcedualMesh(housePlane.GetVertices(), housePlane.GetIndices(), "HousePlane");

            // Update mesh Id
            housePlane.SetMeshId(meshId);
        }
    }

    void RecreateAllWeatherBoards() {
        for (Wall& wall : GetWalls()) {
            wall.RecreateWeatherBoardMesh();
        }
    }

    void RemoveAllWeatherBoards() {
        for (Wall& wall : GetWalls()) {
            wall.CleanUpWeatherBoardMesh();
        }
    }

    void RecreateAllHangingLightCords() {
        for (Light& light : GetLights()) {
            light.ConfigureMeshNodes();
        }
    }

    HouseBvhRegion& GetHouseBvh() {
        return g_houseBvhRegion;
    }
}