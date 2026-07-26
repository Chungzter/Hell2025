#include "EditorWorkspace.h"

#include "EditorViewports.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/World/World.h"

namespace Unloved::EditorSession::Workspace {
    namespace {
        constexpr const char* TEST_HOUSE_NAME = "TestHouse";
        constexpr const char* TEST_MAP_NAME = "Shit";

        EditorSessionMode g_mode = EditorSessionMode::HOUSE;
        uint64_t g_worldGeneration = 0;
        bool g_hasMode = false;

        bool IsWorldCurrent() {
            return g_hasMode && g_worldGeneration == World::GetGeneration();
        }

        bool PrepareHouse() {
            HouseData* houseData = HouseManager::GetHouseDataByName(TEST_HOUSE_NAME);
            if (!houseData) return false;
            Viewports::PrepareInitialView(houseData->GetCreateInfoCollection());
            World::LoadSingleHouse(TEST_HOUSE_NAME);
            if (Player* player = Session::GetLocalPlayerByViewportIndex(0)) {
                if (player->GetFootPosition().y > 10.0f) {
                    player->SetFootPosition(glm::vec3(2.25f, 0.0f, 1.68f));
                    player->GetCamera().SetEulerRotation(glm::vec3(-0.2f, 0.0f, 0.0f));
                }
            }
            return true;
        }

        bool PrepareMap() {
            MapData* mapData = MapManager::GetMapDataByName(TEST_MAP_NAME);
            if (!mapData) return false;

            Viewports::PrepareInitialView(mapData->GetCreateInfoCollection());
            MapCreateInfo mapCreateInfo;
            mapCreateInfo.mapName = TEST_MAP_NAME;
            World::ResetWorld();
            LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });
            World::LoadMapObjects(*mapData, SpawnOffset());
            return true;
        }

        bool Prepare(EditorSessionMode mode) {
            switch (mode) {
                case EditorSessionMode::HOUSE: return PrepareHouse();
                case EditorSessionMode::MAP:   return PrepareMap();
            }
            return false;
        }

        void Commit() {
            switch (g_mode) {
                case EditorSessionMode::HOUSE: HouseManager::UpdateCreateInfoCollectionFromWorld(TEST_HOUSE_NAME); break;
                case EditorSessionMode::MAP:   MapManager::UpdateCreateInfoCollectionFromWorld(TEST_MAP_NAME);     break;
            }
        }
    }

    bool Open(EditorSessionMode mode) {
        if (g_hasMode && g_mode == mode && IsWorldCurrent()) return true;
        if (!Prepare(mode)) return false;

        g_mode = mode;
        g_worldGeneration = World::GetGeneration();
        g_hasMode = true;
        return true;
    }

    void Close() {
        if (!IsWorldCurrent()) return;
        Commit();
    }

    void Save() {
        if (!IsWorldCurrent()) return;
        Commit();

        switch (g_mode) {
            case EditorSessionMode::HOUSE:
                HouseManager::SaveHouse(TEST_HOUSE_NAME);
                Debug::BlitQuickDebugMessage("House saved");
                break;
            case EditorSessionMode::MAP:
                MapManager::SaveMap(TEST_MAP_NAME);
                Debug::BlitQuickDebugMessage("Map saved");
                break;
        }
    }

    void Discard() {
        g_hasMode = false;
        g_worldGeneration = 0;
    }

    bool HasMode() {
        return g_hasMode;
    }

    EditorSessionMode GetMode() {
        return g_mode;
    }
}
