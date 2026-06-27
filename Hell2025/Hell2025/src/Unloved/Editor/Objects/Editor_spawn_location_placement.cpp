#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"

#include "Unloved/Maps/MapManager.h"
#include "Legacy/Renderer/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Editor/Editor.h"

namespace Input = Hell::Input;

namespace Unloved::Editor {

    void UpdatePlayerCampaignSpawnPlacement() {
        Map* map = MapManager::GetMapByName(GetEditorMapName());
        if (!map) return;

        if (Input::LeftMousePressed()) {
            PhysXRayResult result = GetMouseRayPhsyXHitPosition();
            if (result.hitFound) {
                map->AddPlayerCampaignSpawn(result.hitPosition);
                LegacyWorld::UpdateWorldSpawnPointsFromMap(map);
                ExitObjectPlacement();
                Logging::Debug() << "Added player campaign spawn: " << result.hitPosition;
            }
        }
    }

    void UpdatePlayerDeathmatchSpawnPlacement() {
        Map* map = MapManager::GetMapByName(GetEditorMapName());
        if (!map) return;

        if (Input::LeftMousePressed()) {
            PhysXRayResult result = GetMouseRayPhsyXHitPosition();
            if (result.hitFound) {
                map->AddPlayerDeathmatchSpawn(result.hitPosition);
                LegacyWorld::UpdateWorldSpawnPointsFromMap(map);
                Logging::Debug() << "Added player deathmatch spawn: " << result.hitPosition;
                ExitObjectPlacement();
            }
        }
    }
}