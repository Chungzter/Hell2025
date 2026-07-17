#include "LegacyWorld.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Types.h"

using namespace Hell;

namespace Unloved::LegacyWorld {

    std::vector<HeightMapChunk> g_heightMapChunks;
    std::vector<Map> g_maps;
    std::vector<Road> g_roads;

    std::vector<GPULight> g_gpuLightsLowRes;
    std::vector<GPULight> g_gpuLightsMidRes;
    std::vector<GPULight> g_gpuLightsHighRes;

    std::map<ivecXZ, int> g_validChunks;

    std::string g_mapName = "";
    uint32_t g_worldMapChunkCountX = 0;
    uint32_t g_worldMapChunkCountZ = 0;

    // HACK!
    float g_runTime = 0.0f;
    bool g_playersAwaitingRespawn = false;
    // HACK!

    struct WorldState {
        bool oceanEnabled = true;
    } g_worldState;

    void Init() {

        NewRun();

        //if (GetRoads().size() == 0) {
        //    Road& road = GetRoads().emplace_back();
        //    road.Init();
        //}
    }

    void LoadMap(const std::string& mapName) {
        //ResetWorld();

        MapCreateInfo createInfo;
        createInfo.mapName = mapName;
        createInfo.spawnOffsetChunkX = 0;
        createInfo.spawnOffsetChunkZ = 0;

        LoadMaps({ createInfo });
    }

    void LoadMaps(std::vector<MapCreateInfo> mapCreateInfoSet) {
        LoadMapsHeightMapData(mapCreateInfoSet);

        int i = 0;
        for (MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            SpawnOffset spawnOffset;
            spawnOffset.translation.x = mapCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
            spawnOffset.translation.z = mapCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;

           //if (i == 1) {
           //    spawnOffset.translation.x = 32 * mapCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
           //    spawnOffset.translation.z = 32 * mapCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
           //}

            LoadMapObjects(mapCreateInfo.mapName, spawnOffset);
            LoadMapHouses(mapCreateInfo.mapName, spawnOffset);

            i++;
        }

        KangarooCreateInfo kangarooCreateInfo;
        kangarooCreateInfo.position = glm::vec3(48, 32.6, 39);
        kangarooCreateInfo.rotation = glm::vec3(0, HELL_PI * -0.5f, 0);
        Unloved::World::AddKangaroo(kangarooCreateInfo);
    }

    void LoadMapsHeightMapData(std::vector<MapCreateInfo> mapCreateInfoSet) {
        g_maps.clear();
        g_worldMapChunkCountX = 0;
        g_worldMapChunkCountZ = 0;

        // Load height map data from all maps
        for (MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            int32_t mapIndex = MapManager::GetMapDataIndexByName(mapCreateInfo.mapName);
            MapData* mapData = MapManager::GetMapDataByName(mapCreateInfo.mapName);
            if (!mapData) {
                Logging::Error() << "LegacyWorld::LoadMapsHeightMapData() failed coz '" << mapCreateInfo.mapName << "' was not found";
                return;
            }

            Map& map = g_maps.emplace_back();
            map.m_mapIndex = mapIndex;
            map.spawnOffsetChunkX = mapCreateInfo.spawnOffsetChunkX;
            map.spawnOffsetChunkZ = mapCreateInfo.spawnOffsetChunkZ;

            uint32_t reachX = map.spawnOffsetChunkX + mapData->GetChunkCountX();
            uint32_t reachZ = map.spawnOffsetChunkZ + mapData->GetChunkCountZ();

            g_worldMapChunkCountX = std::max(g_worldMapChunkCountX, reachX);
            g_worldMapChunkCountZ = std::max(g_worldMapChunkCountZ, reachZ);
        }

        // Create heightmap chunks
        g_heightMapChunks.clear();
        g_validChunks.clear();

        // Init heightmap chunks
        int baseVertex = 0;
        int baseIndex = 0;
        for (int x = 0; x < g_worldMapChunkCountX; x++) {
            for (int z = 0; z < g_worldMapChunkCountZ; z++) {
                int cellX = x / 8;
                int cellZ = z / 8;

                HeightMapChunk& chunk = g_heightMapChunks.emplace_back();
                chunk.coord.x = x;
                chunk.coord.z = z;
                chunk.baseVertex = baseVertex;
                chunk.baseIndex = baseIndex;
                baseVertex += VERTICES_PER_CHUNK;
                baseIndex += INDICES_PER_CHUNK;

                g_validChunks[chunk.coord] = g_heightMapChunks.size() - 1;
            }
        }

        Renderer::RecalculateAllHeightMapData(true);
    }

    void LoadSingleHouse(const std::string& houseName) {
        ResetWorld();
        LoadHouse(houseName, SpawnOffset());
    }

    void LoadHouse(const std::string& houseName, SpawnOffset spawnOffset) {
        // TODO: Probably handle me better!
        Ocean::CreatePhysicsPlane();

        HouseData* houseData = HouseManager::GetHouseDataByName(houseName);
        if (!houseData) {
            Logging::Error() << "LegacyWorld::LoadHouse() failed because " << houseName << " was not found";
            return;
        }

        Unloved::World::AddCreateInfoCollection(houseData->GetCreateInfoCollection(), spawnOffset);

        Logging::Debug() << "LegacyWorld::LoadHouse(): " << houseName << " at " << spawnOffset.translation;
    }

    void LoadMapObjects(const std::string& mapName, SpawnOffset spawnOffset) {
        MapData* mapData = MapManager::GetMapDataByName(mapName);
        if (!mapData) {
            Logging::Error() << "LegacyWorld::LoadMapObjects() failed coz '" << mapName << "' was not found";
            return;
        }

        // Add EVERYTHING: doors, walls, draws, toilets, pianos, etc...
        Unloved::World::AddCreateInfoCollection(mapData->GetCreateInfoCollection(), spawnOffset);
    }

    void LoadMapHouses(const std::string& mapName, SpawnOffset spawnOffset) {
        MapData* mapData = MapManager::GetMapDataByName(mapName);
        if (!mapData) {
            Logging::Error() << "LegacyWorld::LoadMapHouses() failed coz '" << mapName << "' was not found";
            return;
        }

        for (const HouseLocation& houseLocation : mapData->GetAdditionalMapData().houseLocations) {
            SpawnOffset houseSpawnOffset = spawnOffset;
            houseSpawnOffset.translation += houseLocation.position;
            houseSpawnOffset.yRotation += houseLocation.rotation;

            LoadHouse("TestHouse", houseSpawnOffset);
        }
    }

    void NewRun() {
        ResetWorld();

        // Respawn roos
        for (Kangaroo& kangaroo : Unloved::World::GetKangaroos()) {
            kangaroo.Respawn();
        }

        // Load the map
        std::vector<MapCreateInfo> mapCreateInfoSet;

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = "Shit";
        mapCreateInfo.spawnOffsetChunkX = 0;
        mapCreateInfo.spawnOffsetChunkZ = 0;
        mapCreateInfoSet.push_back(mapCreateInfo);

        LoadMaps(mapCreateInfoSet);

        // Load a second house close to the first one
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

        // HACK!!!
        g_runTime += Hell::Time::DeltaTime();
        if (g_runTime > 0.2f && g_playersAwaitingRespawn) {
            Unloved::Session::RespawnPlayers();
            g_playersAwaitingRespawn = false;
        }
        // HACK!!!

        for (GameObject& gameObject : Unloved::World::GetGameObjects()) {
            gameObject.BeginFrame();
        }
        //for (Tree& tree : g_trees) {
        //    tree.BeginFrame();
        //}
    }

    void EndFrame() {
        // Nothing as of yet
    }


    void CreateGameObject() {
        Unloved::World::AddGameObject(GameObjectCreateInfo());
    }


    uint64_t CreateAnimatedGameObject() {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::ANIMATED_GAME_OBJECT);
        Unloved::World::GetAnimatedGameObjects().emplace_with_id(id, id);
        return id;
    }

    void ResetWorld() {
        std::cout << "Reset world()\n";

        // Clear height map data
        g_heightMapChunks.clear();
        g_validChunks.clear();
        g_maps.clear();

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        proceduralMeshBuffer.Reset();

        ClearAllObjects();
    }

    void ClearAllObjects() {
        Unloved::HouseBuilder::RemoveAllWeatherBoards();
        Unloved::MirrorManager::CleanUp();
        Unloved::BulletSystem::CleanUp();
        Unloved::BloodSystem::CleanUp();
        Ocean::DestroyPhysicsPlane();
        Unloved::World::CleanUpAll();
        Hell::Physics::FlushPendingRemovals();
    }

    DDGIVolume& GetTestDDGIVolume() {
        static DDGIVolume invalid;

        if (Unloved::World::GetDDGIVolumes().size() > 1) {
            Logging::Fatal() << "LegacyWorld::GetTestDDGIVolume() fucked up, you have more than one LightVolume and ALL your code assumes you only have one\n";
            return invalid;
        }
        if (Unloved::World::GetDDGIVolumes().size() == 1) {
            for (DDGIVolume& ddgiVolume : Unloved::World::GetDDGIVolumes()) {
                return ddgiVolume;
            }
        }
        else {
            Logging::Fatal() << "LegacyWorld::GetTestDDGIVolume() fucked up, you have zero LightVolumes and ALL your code assumes you only have one\n";
            return invalid;
        }

        return invalid;
    }

    void EnableOcean() {
        g_worldState.oceanEnabled = true;
    }

    void DisableOcean() {
        g_worldState.oceanEnabled = false;
    }

    bool HasOcean() {
        return g_worldState.oceanEnabled;
    }

    void AddMap(const std::string& mapName, int32_t spawnOffsetChunkX, int32_t spawnOffsetChunkZ) {

    }

    std::vector<HeightMapChunk>& GetHeightMapChunks() {
        return g_heightMapChunks;
    }

    const uint32_t GetChunkCountX() {
        return g_worldMapChunkCountX;
    }

    const uint32_t GetChunkCountZ() {
        return g_worldMapChunkCountZ;
    }

    const uint32_t GetChunkCount() {
        return (uint32_t)g_heightMapChunks.size();
    }

    bool ChunkExists(int x, int z) {
        return g_validChunks.contains(ivecXZ(x, z));
    }

    const HeightMapChunk* GetChunk(int x, int z) {
        if (!ChunkExists(x, z)) return nullptr;

        int index = g_validChunks[ivecXZ(x, z)];
        return &g_heightMapChunks[index];
    }

    const std::string& GetCurrentMapName() {
        return g_mapName;
    }


    const float GetWorldSpaceWidth() {
        return g_worldMapChunkCountX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
    }

    const float GetWorldSpaceDepth() {
        return g_worldMapChunkCountZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
    }

    void PrintObjectCounts() {
        Logging::Debug()
            << "Doors:          " << Unloved::World::GetDoors().size() << "\n"
            << "Lights:         " << Unloved::World::GetLights().size() << "\n"
            << "Pickups:        " << Unloved::World::GetPickUps().size() << "\n"
            << "Pianos:         " << Unloved::World::GetPianos().size() << "\n"
            << "Picture Frames: " << Unloved::World::GetPictureFrames().size() << "\n"
            << "Planes:         " << Unloved::World::GetWorldPlanes().size() << "\n"
            //<< "Trees:          " << g_trees.size() << "\n"
            << "Walls:          " << Unloved::World::GetWalls().size() << "\n"
            << "Windows:        " << Unloved::World::GetWindows().size() << "\n"
            << "";

    }

    std::vector<Map>& GetMaps()                                         { return g_maps; }
    std::vector<Road>& GetRoads()                                       { return g_roads; }

}
