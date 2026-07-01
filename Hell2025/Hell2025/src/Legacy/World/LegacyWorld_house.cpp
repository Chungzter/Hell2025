#include "LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Systems/DDGI/GlobalIllumination.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/World/World.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "File/JSON.h"

namespace Unloved::LegacyWorld {

    void RecreateAllHouseGeometry() {
        RecreateAllProceduralWallMesh();
        RecreateAllProcedularWorldPlaneMesh();
        RecreateAllWeatherBoards();
        RecreateAllHangingLightCords();
        RecreateAllWallTrims();

        Unloved::WorldBVH::UpdateHouseLightOccluderBvh();

        for (Light& light : Unloved::World::GetLights()) {
            light.RaycastWorldBounds();
        }
    }

    void RecreateAllWallTrims() {
        Hell::SlotMap<TrimSet>& trimSets = Unloved::World::GetTrimSets();
        trimSets.clear();

		for (Wall& wall : Unloved::World::GetWalls()) {
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

			Unloved::World::AddTrimSet(createInfoCeiling, SpawnOffset());
		}
    }

    void RecreateAllProceduralWallMesh() {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

        // Door/window clipping volumes are owned by those objects, so CSG can use them directly

        for (Wall& wall : Unloved::World::GetWalls()) {

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

    void RecreateAllProcedularWorldPlaneMesh() {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

        for (WorldPlane& worldPlane : Unloved::World::GetWorldPlanes()) {
            // Remove old mesh
            meshBuffer.RemoveMesh(worldPlane.GetMeshId());

            // Create new mesh
            uint32_t meshId = meshBuffer.AddMesh(worldPlane.GetVertices(), worldPlane.GetIndices(), "WorldPlane");

            // Update mesh Id
            worldPlane.SetMeshId(meshId);
        }
    }

    void RecreateAllWeatherBoards() {
        for (Wall& wall : Unloved::World::GetWalls()) {
            wall.RecreateWeatherBoardMesh();
        }
    }

    void RemoveAllWeatherBoards() {
        for (Wall& wall : Unloved::World::GetWalls()) {
            wall.CleanUpWeatherBoardMesh();
        }
    }

    void RecreateAllHangingLightCords() {
        for (Light& light : Unloved::World::GetLights()) {
            light.ConfigureMeshNodes();
        }
    }

}
