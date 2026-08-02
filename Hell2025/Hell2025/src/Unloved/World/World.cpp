#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Hell/Common/Constants.h"
#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/Map/MapManager.h"

namespace {
    float g_runTime = 0.0f;
    bool g_playersAwaitingRespawn = false;
    uint64_t g_generation = 0;
}

namespace Unloved::World {

    void NewRun(const std::string& mapName) {
        MapManager::LoadMapData(mapName);
        MapData* mapData = MapManager::GetMapDataByName(mapName);
        if (!mapData) return;

        ResetWorld();

        for (Kangaroo& kangaroo : GetKangaroos()) {
            kangaroo.Respawn();
        }

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = mapName;
        LoadMaps({ mapCreateInfo });

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
        Renderer::WaitIdle();
        LegacyWorld::ResetWorld();
        HouseBuilder::ResetPictureFrameImageList();
        g_generation++;
    }

    void ClearAllObjects() {
        LegacyWorld::ClearAllObjects();
        g_generation++;
    }

    uint64_t GetGeneration() {
        return g_generation;
    }
}
