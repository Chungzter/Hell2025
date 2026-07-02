#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"

#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Editor/Editor.h"
#include "Unloved/World/World.h"

namespace Input = Hell::Input;

namespace Unloved::Editor {

    void UpdatePlayerCampaignSpawnPlacement() {
        MapData* mapData = MapManager::GetMapDataByName(GetEditorMapName());
        if (!mapData) return;

        if (Input::LeftMousePressed()) {
            PhysXRayResult result = GetMouseRayPhsyXHitPosition();
            if (result.hitFound) {
                mapData->AddPlayerCampaignSpawn(result.hitPosition);

                SpawnPointCreateInfo createInfo;
                createInfo.position = result.hitPosition;
                World::AddSpawnPointCampaign(createInfo);

                ExitObjectPlacement();
                Logging::Debug() << "Added player campaign spawn: " << result.hitPosition;
            }
        }
    }

    void UpdatePlayerDeathmatchSpawnPlacement() {
        MapData* mapData = MapManager::GetMapDataByName(GetEditorMapName());
        if (!mapData) return;

        if (Input::LeftMousePressed()) {
            PhysXRayResult result = GetMouseRayPhsyXHitPosition();
            if (result.hitFound) {
                mapData->AddPlayerDeathmatchSpawn(result.hitPosition);

                SpawnPointCreateInfo createInfo;
                createInfo.position = result.hitPosition;
                World::AddSpawnPointDeathMatch(createInfo);

                Logging::Debug() << "Added player deathmatch spawn: " << result.hitPosition;
                ExitObjectPlacement();
            }
        }
    }
}
