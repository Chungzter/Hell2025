#include "LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Systems/DDGI/GlobalIllumination.h"
#include "Unloved/Systems/House/HouseManager.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "File/JSON.h"

namespace Unloved::LegacyWorld {

    void RecreateAllHouseGeometry() {
        RecreateAllProceduralWallMesh();
        RecreateAllProcedularHousePlaneMesh();
        RecreateAllWeatherBoards();
        RecreateAllHangingLightCords();
        RecreateAllWallTrims();

        UpdateHouseLightOccluderBvh();

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

			LegacyWorld::AddTrimSet(createInfoCeiling, SpawnOffset());
		}
    }

    void RecreateAllProceduralWallMesh() {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

        // Door/window clipping volumes are owned by those objects, so CSG can use them directly

        for (Wall& wall : GetWalls()) {

            // Update CSG and trims
            wall.UpdateSegmentsTrimsAndVertexData();

            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                // Remove old mesh
                meshBuffer.RemoveMesh(wallSegment.GetMeshId());

                // Create new mesh
                uint32_t meshId = meshBuffer.AddMesh(wallSegment.GetVertices(), wallSegment.GetIndices(), "WallSegment");

                // Update mesh Id
                wallSegment.SetMeshId(meshId);
            }
        }
    }

    void RecreateAllProcedularHousePlaneMesh() {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

        for (HousePlane& housePlane : GetHousePlanes()) {
            // Remove old mesh
            meshBuffer.RemoveMesh(housePlane.GetMeshId());

            // Create new mesh
            uint32_t meshId = meshBuffer.AddMesh(housePlane.GetVertices(), housePlane.GetIndices(), "HousePlane");

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

}
