#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Hell/Common/Constants.h"
#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Map/MapManager.h"

namespace {
    float g_runTime = 0.0f;
    bool g_playersAwaitingRespawn = false;
}

namespace Unloved::World {

    void Init() {
        NewRun();
    }

    void NewRun() {
        ResetWorld();

        for (Kangaroo& kangaroo : GetKangaroos()) {
            kangaroo.Respawn();
        }

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = "Shit";
        LoadMaps({ mapCreateInfo });

        MapData* mapData = MapManager::GetMapDataByName("Shit");
        if (mapData && !mapData->GetAdditionalMapData().houseLocations.empty()) {
            const HouseLocation& houseLocation = mapData->GetAdditionalMapData().houseLocations.front();

            SpawnOffset secondHouseSpawnOffset;
            secondHouseSpawnOffset.translation = houseLocation.position + glm::vec3(0.0f, 0.0f, 10.0f);
            secondHouseSpawnOffset.yRotation = houseLocation.rotation;
            LoadHouse("TestHouse", secondHouseSpawnOffset);
        }

        Editor::SetEditorMapName(UNDEFINED_STRING);

        g_runTime = 0.0f;
        g_playersAwaitingRespawn = true;
    }

    void BeginFrame() {
        g_runTime += Hell::Time::DeltaTime();
        if (g_runTime > 0.2f && g_playersAwaitingRespawn) {
            Session::RespawnPlayers();
            g_playersAwaitingRespawn = false;
        }

        LegacyWorld::BeginFrame();
    }

    void Update() {
        UpdateEnvironment();
    }

    void EndFrame() {
        LegacyWorld::EndFrame();
    }

    void CleanUp() {
        CleanUpAll();
    }

    void ResetWorld() {
        LegacyWorld::ResetWorld();
    }

    void ClearAllObjects() {
        LegacyWorld::ClearAllObjects();
    }
}
