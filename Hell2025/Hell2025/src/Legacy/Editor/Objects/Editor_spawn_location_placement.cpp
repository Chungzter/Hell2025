#include "Editor/Editor.h"
#include "Hell/Audio.h"
#include "Hell/Logging.h"
#include "Managers/MapManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace Editor {
    void UpdatePlayerCampaignSpawnPlacement() {
        Map* map = MapManager::GetMapByName(GetEditorMapName());
        if (!map) return;

        if (Input::LeftMousePressed()) {
            PhysXRayResult result = GetMouseRayPhsyXHitPosition();
            if (result.hitFound) {
                map->AddPlayerCampaignSpawn(result.hitPosition);
                World::UpdateWorldSpawnPointsFromMap(map);
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
                World::UpdateWorldSpawnPointsFromMap(map);
                Logging::Debug() << "Added player deathmatch spawn: " << result.hitPosition;
                ExitObjectPlacement();
            }
        }
    }
}