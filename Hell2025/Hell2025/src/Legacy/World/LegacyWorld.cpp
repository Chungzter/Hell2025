#include "LegacyWorld.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Time.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Maps/MapManager.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/DDGI/GlobalIllumination.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Types.h"
#include "Util.h"

using namespace Hell;

namespace Unloved::LegacyWorld {

    Hell::SlotMap<AnimatedGameObject> g_animatedGameObjects;
    Hell::SlotMap<ChristmasLightSet> g_christmasLightSets;
    Hell::SlotMap<Unloved::DDGIVolume> g_ddgiVolumes;
    Hell::SlotMap<Door> g_doors;
    Hell::SlotMap<Fence> g_fences;
    Hell::SlotMap<Fireplace> g_fireplaces;
    Hell::SlotMap<GenericObject> g_genericObjects;
    Hell::SlotMap<HousePlane> g_housePlanes;
    Hell::SlotMap<Ladder> g_ladders;
    Hell::SlotMap<PickUp> g_pickUps;
    Hell::SlotMap<PictureFrame> g_pictureFrames;
    Hell::SlotMap<PowerPoleSet> g_powerPoleSets;
    Hell::SlotMap<Staircase> g_staircases;
    Hell::SlotMap<TrimSet> g_trimSets;
    Hell::SlotMap<Wall> g_walls;
    Hell::SlotMap<Window> g_windows;

    std::vector<BulletCasing> g_bulletCasings;
    std::vector<ChristmasTree> g_christmasTrees;
    std::vector<Decal> g_newDecals;
    std::vector<Dobermann> g_dobermanns;
    std::vector<GameObject> g_gameObjects;
    std::vector<Kangaroo> g_kangaroos;
    std::vector<HeightMapChunk> g_heightMapChunks;
    std::vector<Light> g_lights;
    std::vector<MapInstance> g_mapInstances;
    std::vector<Mermaid> g_mermaids;
    std::vector<Piano> g_pianos;
    std::vector<Road> g_roads;
    std::vector<Shark> g_sharks;
    std::vector<SpawnPoint> g_spawnCampaignPoints;
    std::vector<SpawnPoint> g_spawnDeathmatchPoints;
    std::vector<Transform> g_doorAndWindowCubeTransforms;
    //std::vector<Tree> g_trees;

    std::vector<GPUAABB> g_dirtyDoorAABBS;

    // std::unordered_map<uint64_t, HouseInstance> g_houseInstances; // unused???

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

    std::vector<SpawnPoint> g_fallbackSpawnPoints = {
        SpawnPoint(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(-0.162, -HELL_PI * 0.5f, 0)),
        SpawnPoint(glm::vec3(1.5f, 0.0f, 2.0f), glm::vec3(-0.162, -HELL_PI * 0.5f, 0)),
        SpawnPoint(glm::vec3(3.0f, 0.0f, 2.0f), glm::vec3(-0.162, -HELL_PI * 0.5f, 0)),
        SpawnPoint(glm::vec3(4.5f, 0.0f, 2.0f), glm::vec3(-0.162, -HELL_PI * 0.5f, 0))
    };

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

    void LoadMapInstance(const std::string& mapName) {
        //ResetWorld();

        MapInstanceCreateInfo createInfo;
        createInfo.mapName = mapName;
        createInfo.spawnOffsetChunkX = 0;
        createInfo.spawnOffsetChunkZ = 0;

        LoadMapInstances({ createInfo });
    }

    void LoadMapInstances(std::vector<MapInstanceCreateInfo> mapInstanceCreateInfoSet) {
        LoadMapInstancesHeightMapData(mapInstanceCreateInfoSet);

        int i = 0;
        for (MapInstanceCreateInfo& mapInstanceCreateInfo : mapInstanceCreateInfoSet) {
            SpawnOffset spawnOffset;
            spawnOffset.translation.x = mapInstanceCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
            spawnOffset.translation.z = mapInstanceCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;

           //if (i == 1) {
           //    spawnOffset.translation.x = 32 * mapInstanceCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
           //    spawnOffset.translation.z = 32 * mapInstanceCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
           //}

            // Load the objects
            LoadMapInstanceObjects(mapInstanceCreateInfo.mapName, spawnOffset);
            LoadMapInstanceHouses(mapInstanceCreateInfo.mapName, spawnOffset);

            Logging::Warning() << "MAKE SURE YOU REMOVE THIS LINE break IT IS DISABLING THE LOAD OF THE SECOND MAP INSTANCE";
            break;
            i++;
        }

        RecreateAllHouseGeometry();

        GameObjectCreateInfo createInfo;
        createInfo.position = glm::vec3(32.45f, 30.52f, 10.22f);
        createInfo.rotation.y = -HELL_PI * 0.5f;
        createInfo.scale = glm::vec3(1.0f);
        createInfo.modelName = "Reflector";
        AddGameObject(createInfo);
        g_gameObjects[0].SetMeshMaterial("ReflectorPole", "Fence");
        g_gameObjects[0].SetMeshMaterial("ReflectorRed", "Red");

        DobermannCreateInfo dobermannCreateInfo;
        dobermannCreateInfo.position = glm::vec3(37.2f, 31.0f, 35.3f);
        AddDobermann(dobermannCreateInfo);

        KangarooCreateInfo kangarooCreateInfo;
        kangarooCreateInfo.position = glm::vec3(48, 32.6, 39);
        kangarooCreateInfo.rotation = glm::vec3(0, HELL_PI * -0.5f, 0);
        AddKangaroo(kangarooCreateInfo);

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);
        Hell::Physics::SpawnRagdoll(glm::vec3(36, 31, 36), glm::vec3(0.0f, 0.2f, 0.0f), "manikin", Unloved::GetNextObjectId(ObjectType::RAGDOLL_STANDALONE), filterData);
        Hell::Physics::SpawnRagdoll(glm::vec3(37, 31, 36), glm::vec3(0.0f, -0.4f, 0.0f), "manikin", Unloved::GetNextObjectId(ObjectType::RAGDOLL_STANDALONE), filterData);
    }

    void LoadMapInstancesHeightMapData(std::vector<MapInstanceCreateInfo> mapInstanceCreateInfoSet) {
        g_mapInstances.clear();
        g_worldMapChunkCountX = 0;
        g_worldMapChunkCountZ = 0;

        // Load height map data from all map instances
        for (MapInstanceCreateInfo& mapInstanceCreateInfo : mapInstanceCreateInfoSet) {
            int32_t mapIndex = MapManager::GetMapIndexByName(mapInstanceCreateInfo.mapName);
            Map* map = MapManager::GetMapByName(mapInstanceCreateInfo.mapName);
            if (!map) {
                Logging::Error() << "LegacyWorld::LoadMapInstancesHeightMapData() failed coz '" << mapInstanceCreateInfo.mapName << "' was not found";
                return;
            }

            MapInstance& mapInstance = g_mapInstances.emplace_back();
            mapInstance.m_mapIndex = mapIndex;
            mapInstance.spawnOffsetChunkX = mapInstanceCreateInfo.spawnOffsetChunkX;
            mapInstance.spawnOffsetChunkZ = mapInstanceCreateInfo.spawnOffsetChunkZ;

            uint32_t reachX = mapInstance.spawnOffsetChunkX + map->GetChunkCountX();
            uint32_t reachZ = mapInstance.spawnOffsetChunkZ + map->GetChunkCountZ();

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
        LoadHouseInstance(houseName, SpawnOffset());
        RecreateAllHouseGeometry();
    }

    void LoadHouseInstance(const std::string& houseName, SpawnOffset spawnOffset) {

        // TODO: Probably handle me better!
        Ocean::CreatePhysicsPlane();

        House* house = HouseManager::GetHouseByName(houseName);
        if (!house) {
            Logging::Error() << "LegacyWorld::LoadHouseInstance() failed because " << houseName << " was not found";
            return;
        }

        CreateInfoCollection& createInfoCollection = house->GetCreateInfoCollection();
        AddCreateInfoCollection(createInfoCollection, spawnOffset);


        MermaidCreateInfo mermaidCreateInfo;

        mermaidCreateInfo.position = glm::vec3(14.0f, 29.0f, 36.5f); // outdoors

        mermaidCreateInfo.position = glm::vec3(36.0f, 31.0f, 36.5f); // indoors
        mermaidCreateInfo.rotation.y = 0.05f;                        // indoors

        AddMermaid(mermaidCreateInfo);


        // Add shark
        for (Shark& shark : GetSharks()) {
            shark.CleanUp();
        }
        g_sharks.clear();

        Shark& shark = g_sharks.emplace_back();
        shark.Init(glm::vec3(5.0f, 28.85f, 40.0f));

        Logging::Debug() << "LegacyWorld::LoadHouseInstance(): " << houseName << " at " << spawnOffset.translation;
    }

    void LoadMapInstanceObjects(const std::string& mapName, SpawnOffset spawnOffset) {
        Map* map = MapManager::GetMapByName(mapName);
        if (!map) {
            Logging::Error() << "LegacyWorld::LoadMapInstanceObjects() failed coz '" << mapName << "' was not found";
            return;
        }

        // Add EVERYTHING: doors, walls, draws, toilets, pianos, etc...
        AddCreateInfoCollection(map->GetCreateInfoCollection(), spawnOffset);

        // Load campaign spawn points
        for (SpawnPoint& spawnPoint : map->GetAdditionalMapData().playerCampaignSpawns) {
            SpawnPoint& addedSpawnPoint = g_spawnCampaignPoints.emplace_back(spawnPoint);
            addedSpawnPoint.Init();
        }

        // Load deathmatch spawn points
        for (SpawnPoint& spawnPoint : map->GetAdditionalMapData().playerDeathmatchSpawns) {
            SpawnPoint& addedSpawnPoint = g_spawnDeathmatchPoints.emplace_back(spawnPoint);
            addedSpawnPoint.Init();
        }
    }

    void LoadMapInstanceHouses(const std::string& mapName, SpawnOffset spawnOffset) {
        Map* map = MapManager::GetMapByName(mapName);
        if (!map) {
            Logging::Error() << "LegacyWorld::LoadMapInstanceHouses() failed coz '" << mapName << "' was not found";
            return;
        }

        for (HouseLocation& houseLocation : map->GetAdditionalMapData().houseLocations) {
            SpawnOffset houseSpawnOffset = spawnOffset;
            houseSpawnOffset.translation += houseLocation.position;
            houseSpawnOffset.yRotation += houseLocation.rotation;

            LoadHouseInstance("TestHouse", houseSpawnOffset);
        }
    }

    void NewRun() {
        ResetWorld();

        // Respawn roos
        for (Kangaroo& kangaroo : g_kangaroos) {
            kangaroo.Respawn();
        }

        // Load two instances of the map
        std::vector<MapInstanceCreateInfo> mapInstanceCreateInfoSet;

        MapInstanceCreateInfo mapInstanceCreateInfo;
        mapInstanceCreateInfo.mapName = "Shit";
        mapInstanceCreateInfo.spawnOffsetChunkX = 0;
        mapInstanceCreateInfo.spawnOffsetChunkZ = 0;
        mapInstanceCreateInfoSet.push_back(mapInstanceCreateInfo);

        //mapInstanceCreateInfo.mapName = "Shit";
        //mapInstanceCreateInfo.spawnOffsetChunkX = 8;
        //mapInstanceCreateInfo.spawnOffsetChunkZ = 4;
        //mapInstanceCreateInfoSet.push_back(mapInstanceCreateInfo);

        LoadMapInstances(mapInstanceCreateInfoSet);

        Editor::SetEditorMapName(UNDEFINED_STRING);

        g_runTime = 0.0f;
        g_playersAwaitingRespawn = true;

        //Update(0.0f);
        //Hell::Physics::ForceZeroStepUpdate();
        //Update(0.0f);
        //Hell::Physics::ForceZeroStepUpdate();
        //Game::RespawnPlayers();
        //Update(0.0f);
        //Hell::Physics::ForceZeroStepUpdate();
        //Update(0.0f);
        //Hell::Physics::ForceZeroStepUpdate();
    }

    void BeginFrame() {

        // HACK!!!
        g_runTime += Hell::Time::DeltaTime();
        if (g_runTime > 0.2f && g_playersAwaitingRespawn) {
            Unloved::Session::RespawnPlayers();
            g_playersAwaitingRespawn = false;
        }
        // HACK!!!

        for (GameObject& gameObject : g_gameObjects) {
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
        g_gameObjects.emplace_back();
    }


    const glm::vec3& GetObjectPosition(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);

        if (Unloved::DDGIVolume* object = GetDDGIVolumeByObjectId(objectId)) return object->GetOrigin();
        // etc

        Logging::Warning() << "LegacyWorld::GetObjectPosition(..) failed for ID " << objectId << ". You haven't implemented " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n";
        return invalid;
    }

    const glm::vec3& GetObjectRotation(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);

        if (Unloved::DDGIVolume* object = GetDDGIVolumeByObjectId(objectId)) return object->GetRotation();
        // etc

        Logging::Warning() << "LegacyWorld::GetObjectRotation(..) failed for ID " << objectId << ". You haven't implemented " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n";
        return invalid;
    }

    const std::string& GetObjectEditorName(uint64_t objectId) {
        const static std::string invalid = UNDEFINED_STRING;

        if (Unloved::DDGIVolume* object = GetDDGIVolumeByObjectId(objectId)) return object->GetEditorName();
        if (Door* object = GetDoorByObjectId(objectId))             return object->GetEditorName();
        // etc

        return invalid;
    }

    uint64_t CreateAnimatedGameObject() {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::ANIMATED_GAME_OBJECT);
        g_animatedGameObjects.emplace_with_id(id, id);
        return id;
    }

    AnimatedGameObject* GetAnimatedGameObjectByObjectId(uint64_t objectId) {
        return g_animatedGameObjects.get(objectId);
    }

    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId) {
        return g_christmasLightSets.get(objectId);
    }

    Unloved::DDGIVolume* GetDDGIVolumeByObjectId(uint64_t id) {
        return g_ddgiVolumes.get(id);
    }

    Door* GetDoorByObjectId(uint64_t objectId) {
        return g_doors.get(objectId);
    }

    GenericObject* GetGenericObjectById(uint64_t objectId) {
        return g_genericObjects.get(objectId);
    }

    Fence* GetFenceByObjectId(uint64_t objectId) {
        return g_fences.get(objectId);
    }

    Fireplace* GetFireplaceById(uint64_t objectId) {
        return g_fireplaces.get(objectId);
    }

    HousePlane* GetHousePlaneByObjectId(uint64_t objectId) {
        return g_housePlanes.get(objectId);
    }

    Ladder* GetLadderByObjectId(uint64_t objectId) {
        return g_ladders.get(objectId);
    }

    PickUp* GetPickUpByObjectId(uint64_t objectId) {
        return g_pickUps.get(objectId);
    }

    PowerPoleSet* GetPowerPoleSetByObjectId(uint64_t objectId) {
        return g_powerPoleSets.get(objectId);
    }

    Staircase* GetStaircaseByObjectId(uint64_t objectId) {
        return g_staircases.get(objectId);
    }

    Wall* GetWallByObjectId(uint64_t objectId) {
        return g_walls.get(objectId);
    }

    GameObject* GetGameObjectByName(const std::string& name) {
        for (GameObject& gameObject : g_gameObjects) {
            if (gameObject.m_name == name) {
                return &gameObject;
            }
        }
        return nullptr;
    }

    GameObject* GetGameObjectByIndex(int32_t index) {
        if (index >= 0 && index < g_gameObjects.size()) {
            return &g_gameObjects[index];
        }
        else {
            return nullptr;
        }
    }

    Light* GetLightByIndex(int32_t index) {
        if (index >= 0 && index < g_lights.size()) {
            return &g_lights[index];
        }
        else {
            std::cout << "LegacyWorld::GetLightByIndex() failed: index " << index << " out of range of size " << g_lights.size() << "\n";
            return nullptr;
        }
    }

    Light* GetLightByObjectId(uint64_t objectId) {
        for (Light& light : g_lights) {
            if (light.GetObjectId() == objectId) {
                return &light;
            }
        }
        return nullptr;
    }

    //Tree* GetTreeByIndex(int32_t index) {
    //    if (index >= 0 && index < g_trees.size()) {
    //        return &g_trees[index];
    //    }
    //    else {
    //        return nullptr;
    //    }
    //}

    PianoKey* GetPianoKeyByObjectId(uint64_t objectId) {
        for (Piano& piano : LegacyWorld::GetPianos()) {
            if (piano.PianoKeyExists(objectId)) {
                return piano.GetPianoKey(objectId);
            }
        }
        return nullptr;
    }

    PictureFrame* GetPictureFrameByObjectId(uint64_t objectId) {
        for (PictureFrame& pictureFrame : LegacyWorld::GetPictureFrames()) {
            if (pictureFrame.GetObjectId() == objectId) {
                return &pictureFrame;
            }
        }
        return nullptr;
    }

    Wall* GetWallByWallSegmentObjectId(uint64_t objectId) {
        for (Wall& wall : g_walls) {
            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                if (wallSegment.GetObjectId() == objectId) {
                    return &wall;
                }
            }
        }
        return nullptr;
    }

    Piano* GetPianoByMeshNodeObjectId(uint64_t objectId) {
        for (Piano& piano : g_pianos) {

            const MeshNodes& meshNodes = piano.GetMeshNodes();
            if (meshNodes.HasNodeWithObjectId(objectId)) {
                return &piano;
            }
        }
        return nullptr;
    }

    void SetObjectPosition(uint64_t objectId, const glm::vec3& position) {

        if (Unloved::DDGIVolume* object = LegacyWorld::GetDDGIVolumeByObjectId(objectId)) object->SetOrigin(position);

        if (Door* door = LegacyWorld::GetDoorByObjectId(objectId)) {
            door->SetPosition(position);
            RecreateAllHouseGeometry();
            Hell::Physics::ForceZeroStepUpdate();
        }

        if (GenericObject* genericObject = LegacyWorld::GetGenericObjectById(objectId)) {
            genericObject->SetPosition(position);
        }

        if (Fireplace* fireplace= LegacyWorld::GetFireplaceById(objectId)) {
            fireplace->SetPosition(position);
        }

        if (Piano* piano = LegacyWorld::GetPianoByObjectId(objectId)) {
            piano->SetPosition(position);
            Hell::Physics::ForceZeroStepUpdate();
        }

        if (HousePlane* plane = LegacyWorld::GetHousePlaneByObjectId(objectId)) {
            plane->UpdateWorldSpaceCenter(position);
            RecreateAllHouseGeometry();
        }

        if (Ladder* ladder = LegacyWorld::GetLadderByObjectId(objectId)) {
            ladder->SetPosition(position);
        }

        if (Light* light = LegacyWorld::GetLightByObjectId(objectId)) {
            light->SetPosition(position);
        }

        if (PickUp* pickUp = LegacyWorld::GetPickUpByObjectId(objectId)) {
            pickUp->SetPosition(position);
        }

        if (PictureFrame* pictureFrame = LegacyWorld::GetPictureFrameByObjectId(objectId)) {
            pictureFrame->SetPosition(position);
        }

        if (Staircase* staircase = LegacyWorld::GetStaircaseByObjectId(objectId)) {
            staircase->SetPosition(position);
        }

        //if (Tree* tree = LegacyWorld::GetTreeByObjectId(objectId)) {
        //    tree->SetPosition(position);
        //}

        if (Wall* wall = LegacyWorld::GetWallByObjectId(objectId)) {
            wall->UpdateWorldSpaceCenter(position);
            Hell::Physics::ForceZeroStepUpdate();
            RecreateAllHouseGeometry();
        }

        if (Window* window = LegacyWorld::GetWindowByObjectId(objectId)) {
            window->SetPosition(position);
            RecreateAllHouseGeometry();
            Hell::Physics::ForceZeroStepUpdate();
        }
    }

    void SetObjectRotation(uint64_t objectId, const glm::vec3& rotation) {
        if (Unloved::DDGIVolume* object = LegacyWorld::GetDDGIVolumeByObjectId(objectId)) object->SetRotation(rotation);

        if (Fireplace* object = LegacyWorld::GetFireplaceById(objectId)) {
            object->SetRotation(rotation);
        }
        if (GenericObject* object = LegacyWorld::GetGenericObjectById(objectId)) {
            object->SetRotation(rotation);
        }
        if (Ladder* object = LegacyWorld::GetLadderByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
        if (PickUp* object = LegacyWorld::GetPickUpByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
        if (Staircase* object = LegacyWorld::GetStaircaseByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
    }

    glm::vec3 GetGizmoOffest(uint64_t objectId) {
        GenericObject* drawers = LegacyWorld::GetGenericObjectById(objectId);
        if (drawers) {
            return drawers->GetGizmoOffset();
        }
        return glm::vec3(0.0f);
    }

    bool RemoveObject(uint64_t objectId) {
        if (objectId == 0) return false;

        if (g_animatedGameObjects.contains(objectId)) {
            g_animatedGameObjects.get(objectId)->CleanUp();
            g_animatedGameObjects.erase(objectId);
            return true;
        }

        if (Unloved::BulletSystem::RemoveBulletTrail(objectId)) {
            return true;
        }

        if (g_christmasLightSets.contains(objectId)) {
            g_christmasLightSets.get(objectId)->CleanUp();
            g_christmasLightSets.erase(objectId);
            return true;
        }

        if (g_doors.contains(objectId)) {
            g_doors.get(objectId)->CleanUp();
            g_doors.erase(objectId);
            return true;
        }

        if (g_fences.contains(objectId)) {
            g_fences.get(objectId)->CleanUp();
            g_fences.erase(objectId);
            return true;
        }

        if (g_fireplaces.contains(objectId)) {
            g_fireplaces.get(objectId)->CleanUp();
            g_fireplaces.erase(objectId);
            return true;
        }

        if (g_genericObjects.contains(objectId)) {
            g_genericObjects.get(objectId)->CleanUp();
            g_genericObjects.erase(objectId);
            return true;
        }

        if (g_housePlanes.contains(objectId)) {
            g_housePlanes.get(objectId)->CleanUp();
            g_housePlanes.erase(objectId);
            return true;
        }

        if (g_powerPoleSets.contains(objectId)) {
            g_powerPoleSets.get(objectId)->CleanUp();
            g_powerPoleSets.erase(objectId);
            return true;
        }

        if (g_staircases.contains(objectId)) {
            g_staircases.get(objectId)->CleanUp();
            g_staircases.erase(objectId);
            return true;
        }

        if (g_trimSets.contains(objectId)) {
            g_trimSets.get(objectId)->CleanUp();
            g_trimSets.erase(objectId);
            return true;
        }

        if (g_pickUps.contains(objectId)) {
            // Dirty any lights within range... maybe put this somewhere else
            PickUp* pickUp = GetPickUpByObjectId(objectId);

            for (Light& light : GetLights()) {
                if (pickUp->GetMeshNodes().m_worldspaceAABB.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                    light.ForceDirty();
                }
            }

            g_pickUps.get(objectId)->CleanUp();
            g_pickUps.erase(objectId);
            return true;
        }

        if (g_pictureFrames.contains(objectId)) {
            g_pictureFrames.get(objectId)->CleanUp();
            g_pictureFrames.erase(objectId);
            return true;
        }

        if (g_ladders.contains(objectId)) {
            g_ladders.get(objectId)->CleanUp();
            g_ladders.erase(objectId);
            return true;
        }

        if (g_walls.contains(objectId)) {
            g_walls.get(objectId)->CleanUp();
            g_walls.erase(objectId);
            return true;
        }

        if (g_windows.contains(objectId)) {
            g_windows.get(objectId)->CleanUp();
            g_windows.erase(objectId);
            return true;
        }

        for (int i = 0; i < g_pianos.size(); i++) {
            if (g_pianos[i].GetObjectId() == objectId) {
                g_pianos[i].CleanUp();
                g_pianos.erase(g_pianos.begin() + i);
                return true;
            }
        }

        //for (int i = 0; i < g_trees.size(); i++) {
        //    if (g_trees[i].GetObjectId() == objectId) {
        //        Logging::Debug() << "Deleted " << g_trees[i].GetEditorName();
        //        g_trees[i].CleanUp();
        //        g_trees.erase(g_trees.begin() + i);
        //        return true;
        //    }
        //}

        Logging::Error() << "LegacyWorld::RemoveObject() Failed to remove object " << objectId << ", check you have implemented type " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n";
        return false;
    }

    void ResetWorld() {
        std::cout << "Reset world()\n";


        // Clear height map data
        g_heightMapChunks.clear();
        g_validChunks.clear();
        g_mapInstances.clear();

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        proceduralMeshBuffer.Reset();

        //RemoveAllHouseBvhs();

        // Cleanup all objects
        ClearAllObjects();


       //AnimatedGameObject* animatedGameObject2 = nullptr;
       //uint64_t id2 = LegacyWorld::CreateAnimatedGameObject();
       //animatedGameObject2 = LegacyWorld::GetAnimatedGameObjectByObjectId(id2);
       //animatedGameObject2->SetSkinnedModel("Knife");
       //animatedGameObject2->SetName("Knife");
       //animatedGameObject2->SetAllMeshMaterials("Knife");
       //animatedGameObject2->PlayAndLoopAnimation("MainLayer", "Knife_Draw", 1.0f);
       //animatedGameObject2->SetScale(0.01);
       //animatedGameObject2->SetPosition(glm::vec3(36, 31, 34));
       //animatedGameObject2->SetMeshMaterialByMeshName("ArmsMale", "Hands");
       //animatedGameObject2->SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
       //
       //
       //AnimatedGameObject* animatedGameObject = nullptr;
       //uint64_t id = LegacyWorld::CreateAnimatedGameObject();
       //animatedGameObject = LegacyWorld::GetAnimatedGameObjectByObjectId(id);
       //animatedGameObject->SetSkinnedModel("Glock");
       //animatedGameObject->SetName("Remington870");
       //animatedGameObject->SetAllMeshMaterials("Glock");
       //animatedGameObject->PlayAndLoopAnimation("MainLayer", "Glock_Reload", 1.0f);
       //animatedGameObject->SetScale(0.01);
       //animatedGameObject->SetPosition(glm::vec3(36, 31, 36));
       //animatedGameObject->SetMeshMaterialByMeshName("ArmsMale", "Hands");
       //animatedGameObject->SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");

        g_gameObjects.clear();

        GameObjectCreateInfo createInfo2;
        createInfo2.position = glm::vec3(49.0f, 31.0f, 39.0f);
        createInfo2.scale = glm::vec3(1.0f);
        createInfo2.modelName = "Bunny";
        AddGameObject(createInfo2);
        g_gameObjects[0].m_meshNodes.SetMeshMaterials("Leopard");
        g_gameObjects[0].SetPosition(glm::vec3(39.0f, 31.0f, 39.0f));
    }

    void ClearAllObjects() {
        RemoveAllWeatherBoards();
        Unloved::MirrorManager::CleanUp();
        Unloved::BulletSystem::CleanUp();
        Unloved::BloodSystem::CleanUp();
        Ocean::DestroyPhysicsPlane();

        for (BulletCasing& bulletCasing : g_bulletCasings)              bulletCasing.CleanUp();
        for (ChristmasLightSet& christmasLights : g_christmasLightSets) christmasLights.CleanUp();
        for (ChristmasTree& christmasTree : g_christmasTrees)           christmasTree.CleanUp();
        for (Unloved::DDGIVolume& object : g_ddgiVolumes)                        object.CleanUp();
        for (Door& door : g_doors)                                      door.CleanUp();
        for (Fireplace& fireplace : g_fireplaces)                       fireplace.CleanUp();
        for (GenericObject& drawer : g_genericObjects)                  drawer.CleanUp();
        for (Fence& fence : g_fences)                                   fence.CleanUp();
        for (GameObject& gameObject : g_gameObjects)                    gameObject.CleanUp();
        //for (Kangaroo& kangaroo : g_kangaroos)                        kangaroo.CleanUp();
        for (Ladder& ladder : g_ladders)                                ladder.CleanUp();
        for (Mermaid& mermaid : g_mermaids)                             mermaid.CleanUp();
        for (HousePlane& housePlane : g_housePlanes)                    housePlane.CleanUp();
        for (Piano& piano : g_pianos)                                   piano.CleanUp();
        for (PickUp& pickUp : g_pickUps)                                pickUp.CleanUp();
        for (PowerPoleSet& powerPoleSet : g_powerPoleSets)               powerPoleSet.CleanUp();
        for (Shark& shark : g_sharks)                                   shark.CleanUp();
        for (Staircase& staircase: g_staircases)                        staircase.CleanUp();
        for (SpawnPoint& spawnPoint : g_spawnCampaignPoints)            spawnPoint.CleanUp();
        for (SpawnPoint& spawnPoint : g_spawnDeathmatchPoints)          spawnPoint.CleanUp();
        //for (Tree& tree : g_trees)                                      tree.CleanUp();
        for (TrimSet& trimSet : g_trimSets)                             trimSet.CleanUp();
        for (Wall& wall : g_walls)                                      wall.CleanUp();
        for (Window& window : g_windows)                                window.CleanUp();

        //for (auto& [id, drawers] : g_drawers) drawers.CleanUp();

        // Clear all containers
        g_bulletCasings.clear();
        g_christmasLightSets.clear();
        g_christmasTrees.clear();
        g_ddgiVolumes.clear();
        g_doors.clear();
        g_fireplaces.clear();
        g_genericObjects.clear();
        g_fences.clear();
        g_gameObjects.clear();
        //g_kangaroos.clear();
        g_ladders.clear();
        g_lights.clear();
        g_mermaids.clear();
        g_pianos.clear();
        g_pickUps.clear();
        g_housePlanes.clear();
        g_pictureFrames.clear();
        g_powerPoleSets.clear();
        g_sharks.clear();
        g_spawnCampaignPoints.clear();
        g_spawnDeathmatchPoints.clear();
        //g_trees.clear();
        g_trimSets.clear();
        g_walls.clear();
        g_windows.clear();
        g_staircases.clear();
    }

    void AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DDGI_VOLUME);
        g_ddgiVolumes.emplace_with_id(id, id, createInfo, spawnOffset);
    }

    void AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DOOR);
        g_doors.emplace_with_id(id, id, createInfo, spawnOffset);
    }

    void AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GENERIC_OBJECT);

        // Assign editor name
        if (createInfo.editorName == UNDEFINED_STRING) {
            createInfo.editorName = Editor::GetNextAvailableGenericObjectName(createInfo.type);
        }

        g_genericObjects.emplace_with_id(id, id, createInfo, spawnOffset);
    }

    void AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WINDOW);
        g_windows.emplace_with_id(id, id, createInfo, spawnOffset);
    }

    void AddHousePlane(HousePlaneCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::HOUSE_PLANE);

        // Assign editor name
        if (createInfo.editorName == UNDEFINED_STRING ||
            createInfo.editorName == "Undefined") {
            createInfo.editorName = Editor::GetNextAvailableHousePlaneName(createInfo.type);
        }

        g_housePlanes.emplace_with_id(id, id, createInfo, spawnOffset);
    }


    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::CHRISTMAS_LIGHTS);
        g_christmasLightSets.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FENCE);
        g_fences.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LADDER);
        g_ladders.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (!Bible::GetItemInfoByName(createInfo.name)) {
            Logging::Warning() << "LegacyWorld::AddPickUp(..) failed: '" << createInfo.name << "' ItemInfo not found in bible";
            return 0;
        }

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICK_UP);
        g_pickUps.emplace_with_id(id, id, createInfo, spawnOffset);

        return id;
    }

    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICTURE_FRAME);
        g_pictureFrames.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::POWER_POLE_SET);
        g_powerPoleSets.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::STAIRCASE);
        g_staircases.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::TRIM_SET);
        g_trimSets.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (createInfo.points.empty()) {
            std::cout << "LegacyWorld::AddWall() failed: createInfo has zero points!\n";
            return 0;
        }

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WALL);

        // Assign editor name
        if (createInfo.editorName == UNDEFINED_STRING) {
            createInfo.editorName = Editor::GetNextAvailableWallName();
        }

        g_walls.emplace_with_id(id, id, createInfo, spawnOffset);

        return id;
    }


    void AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FIREPLACE);
        g_fireplaces.emplace_with_id(id, id, createInfo, spawnOffset);
    }

    void AddBulletCasing(BulletCasingCreateInfo createInfo, SpawnOffset spawnOffset) {
        createInfo.position += spawnOffset.translation;
        g_bulletCasings.push_back(BulletCasing(createInfo));
    }

    void AddDecal2(DecalCreateInfo createInfo) {
        g_newDecals.push_back(Decal(createInfo));
    }

    void AddDobermann(DobermannCreateInfo& createInfo) {
        Dobermann& dobermann = g_dobermanns.emplace_back();
        dobermann.Init(createInfo);
    }

    void AddKangaroo(const KangarooCreateInfo& createInfo) {
        Kangaroo& kangaroo = g_kangaroos.emplace_back();
        kangaroo.Init(createInfo);
    }

    void AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset) {
        g_christmasTrees.push_back(ChristmasTree(createInfo, spawnOffset));
    }

    void AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        createInfo.position += spawnOffset.translation;
        g_gameObjects.push_back(GameObject(createInfo));
    }

    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset) {
        uint64_t id = Unloved::GetNextObjectId(ObjectType::LIGHT);

        //createInfo.position += spawnOffset.translation;
        g_lights.emplace_back(Light(id, createInfo, spawnOffset));

        return id;
    }

    void AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset) {
        Mermaid& mermaid = g_mermaids.emplace_back();
        mermaid.Init(createInfo, spawnOffset);
    }

    void AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset) {
        createInfo.position += spawnOffset.translation;
        if (createInfo.soundFontName == UNDEFINED_STRING) {
            createInfo.soundFontName = "YamahaGrandLiteV2";
        }

        Piano& piano = g_pianos.emplace_back();
        piano.Init(createInfo);
    }

    //void AddTree(TreeCreateInfo createInfo, SpawnOffset spawnOffset) {
    //    Logging::Warning() << "LegacyWorld::AddTree(...) failed cause you removed the that did it, to stop some whack crash";
    //    createInfo.position += spawnOffset.translation;
    //
    //    if (createInfo.editorName == UNDEFINED_STRING) {
    //        createInfo.editorName = Editor::GetNextAvailableTreeName(createInfo.type);
    //    }
    //    g_trees.push_back(Tree(createInfo));
    //}

    SpawnPoint GetRandomCampaignSpawnPoint() {
        SpawnPoint spawnPoint;
        if (g_spawnCampaignPoints.size()) {
            int rand = Util::RandomInt(0, g_spawnCampaignPoints.size() - 1); g_spawnCampaignPoints[rand];
            spawnPoint = g_spawnCampaignPoints[rand];
        }
        else {
            int rand = Util::RandomInt(0, g_fallbackSpawnPoints.size() - 1); g_fallbackSpawnPoints[rand];
            spawnPoint = g_fallbackSpawnPoints[rand];
        }

        g_spawnCampaignPoints.clear();
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(43.9485, 32.6516, 36.7408), glm::vec3(-0.294, -5.002, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(40.3495, 32.6486, 34.1408), glm::vec3(-0.168, -9.482, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(42.6229, 32.6482, 41.4889), glm::vec3(-0.282, -11.772, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(34.7497, 35.452, 37.4222), glm::vec3(-0.206, -15.736, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(34.9035, 32.6505, 39.5006), glm::vec3(-0.146, -14.242, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(34.8531, 32.6496, 33.6023), glm::vec3(-0.258, -15.138, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(33.3506, 32.6481, 41.131), glm::vec3(-0.166, -18.282, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(57.3242, 33.5911, 48.8959), glm::vec3(-0.134, -18.1, 0)));
        g_spawnCampaignPoints.push_back(SpawnPoint(glm::vec3(40.095, 32.4311, 31.6613), glm::vec3(-0.11, -14.256, 0)));

        // Check you didn't just spawn on another player
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            float distanceToOtherPlayer = glm::distance(spawnPoint.GetPosition(), player->GetFootPosition());
            if (distanceToOtherPlayer < 1.0f) {
                return GetRandomCampaignSpawnPoint();
            }
        }

        return spawnPoint;
    }

    SpawnPoint GetRandomDeathmanSpawnPoint() {
        SpawnPoint spawnPoint;
        if (g_spawnDeathmatchPoints.size()) {
            int rand = Util::RandomInt(0, g_spawnDeathmatchPoints.size() - 1); g_spawnDeathmatchPoints[rand];
            spawnPoint = g_spawnDeathmatchPoints[rand];
        }
        else {
            int rand = Util::RandomInt(0, g_fallbackSpawnPoints.size() - 1); g_fallbackSpawnPoints[rand];
            spawnPoint = g_fallbackSpawnPoints[rand];
        }

        // Check you didn't just spawn on another player
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            float distanceToOtherPlayer = glm::distance(spawnPoint.GetPosition(), player->GetFootPosition());
            if (distanceToOtherPlayer < 1.0f) {
                return GetRandomCampaignSpawnPoint();
            }
        }

        return spawnPoint;
    }

    void UpdateWorldSpawnPointsFromMap(Map* map) {
        if (!map) {
            Logging::Error() << "LegacyWorld::UpdateWorldSpawnPointsFromMap() failed coz map param was nullptr";
            return;
        }
        g_spawnCampaignPoints = map->GetAdditionalMapData().playerCampaignSpawns;
        g_spawnDeathmatchPoints = map->GetAdditionalMapData().playerDeathmatchSpawns;
    }


    Unloved::DDGIVolume& GetTestDDGIVolume() {
        static Unloved::DDGIVolume invalid;

        if (LegacyWorld::GetDDGIVolumes().size() > 1) {
            Logging::Fatal() << "LegacyWorld::GetTestDDGIVolume() fucked up, you have more than one LightVolume and ALL your code assumes you only have one\n";
            return invalid;
        }
        if (LegacyWorld::GetDDGIVolumes().size() == 1) {
            for (Unloved::DDGIVolume& ddgiVolume : LegacyWorld::GetDDGIVolumes()) {
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

    void AddMapInstance(const std::string& mapName, int32_t spawnOffsetChunkX, int32_t spawnOffsetChunkZ) {

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
            << "Doors:          " << g_doors.size() << "\n"
            << "Lights:         " << g_lights.size() << "\n"
            << "Pickups:        " << g_pickUps.size() << "\n"
            << "Pianos:         " << g_pianos.size() << "\n"
            << "Picture Frames: " << g_pictureFrames.size() << "\n"
            << "Planes:         " << g_housePlanes.size() << "\n"
            //<< "Trees:          " << g_trees.size() << "\n"
            << "Walls:          " << g_walls.size() << "\n"
            << "Windows:        " << g_windows.size() << "\n"
            << "";

    }

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t id, int32_t meshNodeLocalIndex) {
        if (meshNodeLocalIndex < 0) return nullptr;

        if (Door* object = GetDoorByObjectId(id))               return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Fireplace* object = GetFireplaceById(id))           return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (GenericObject* object = GetGenericObjectById(id))   return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Piano* object = GetPianoByObjectId(id))             return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Window* object = GetWindowByObjectId(id))           return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);

        return nullptr;
    }

    //BlendingMode GetBlendingModeByObjectIdAndMeshNodeLocalIndex(uint64_t id, int32_t meshNodeLocalIndex) {
    //    if (meshNodeLocalIndex < 0) {
    //        return BlendingMode::UNDEFINED;
    //    }
    //
    //    MeshNodes* meshNodes = nullptr;
    //
    //    if (Door* door = GetDoorByObjectId(id)) {
    //        meshNodes = &door->GetMeshNodes();
    //    }
    //    else if (Fireplace* fireplace = GetFireplaceById(id)) {
    //        meshNodes = &fireplace->GetMeshNodes();
    //    }
    //    else if (GenericObject* genericObject = GetGenericObjectById(id)) {
    //        meshNodes = &genericObject->GetMeshNodes();
    //    }
    //    else if (Piano* piano = GetPianoByObjectId(id)) {
    //        meshNodes = &piano->GetMeshNodes();
    //    }
    //    else if (Window* window = GetWindowByObjectId(id)) {
    //        meshNodes = &window->GetMeshNodes();
    //    }
    //    else {
    //        Logging::Warning() << "LegacyWorld::GetBlendingModeByObjectIdAndMeshNodeLocalIndex(...) failed: unknown object type\n";
    //        return BlendingMode::UNDEFINED;
    //    }
    //
    //    // Safe to retrieve the blending mode now
    //    if (MeshNode* meshNode = meshNodes->GetMeshNodeByLocalIndex(meshNodeLocalIndex)) {
    //        return meshNode->blendingMode;
    //    }
    //}

    Piano* GetPianoByObjectId(uint64_t objectId) {
        for (Piano& piano : g_pianos) {
            if (piano.GetObjectId() == objectId) {
                return &piano;
            }
        }
        return nullptr;
    }

    Shark* GetSharkByObjectId(uint64_t objectId) {
        for (Shark& shark: g_sharks) {
            if (shark.GetObjectId() == objectId) {
                return &shark;
            }
        }
        return nullptr;
    }

    Dobermann* GetDobermannByObjectId(uint64_t objectId) {
        for (Dobermann& dobermann : g_dobermanns) {
            CharacterController* characterController = dobermann.GetCharacterController();
            if (!characterController) continue;

            auto* pxController = characterController->GetPxController();
            if (!pxController) continue;

            auto* pxActor = pxController->getActor();
            if (!pxActor) continue;

            PhysicsUserData* userData = static_cast<PhysicsUserData*>(pxActor->userData);
            if (userData && userData->objectId == objectId) {
                return &dobermann;
            }
        }
        return nullptr;
    }

    Kangaroo* GetKangarooByObjectId(uint64_t objectId) {
        for (Kangaroo& kangaroo : g_kangaroos) {
            if (kangaroo.GetObjectId() == objectId) {
                return &kangaroo;
            }
        }
        return nullptr;
    }

    Window* GetWindowByObjectId(uint64_t objectId) {
        for (Window& window : g_windows) {
            if (window.GetObjectId() == objectId) {
                return &window;
            }
        }
        return nullptr;
    }

    std::vector<uint64_t> GetLightIds() {
        static std::vector<uint64_t> ids;
        ids.clear();
        ids.reserve(g_lights.size());

        for (Light& light : LegacyWorld::GetLights()) {
            if (light.GetType() != LightType::FIREPLACE_FIRE) {
                ids.push_back(light.GetObjectId());
            }
        }

        return ids;
    }

    uint32_t GetLightCount()                                     { return (uint32_t)g_lights.size(); }


    Hell::SlotMap<AnimatedGameObject>& GetAnimatedGameObjects() { return g_animatedGameObjects; }
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets()   { return g_christmasLightSets; }
    Hell::SlotMap<Unloved::DDGIVolume>& GetDDGIVolumes()                 { return g_ddgiVolumes; }
    Hell::SlotMap<Door>& GetDoors()                             { return g_doors; }
    Hell::SlotMap<GenericObject>& GetGenericObjects()           { return g_genericObjects; }
    Hell::SlotMap<Fence>& GetFences()                           { return g_fences; }
    Hell::SlotMap<Fireplace>& GetFireplaces()                   { return g_fireplaces; }
    Hell::SlotMap<HousePlane>& GetHousePlanes()                 { return g_housePlanes; }
    Hell::SlotMap<Ladder>& GetLadders()                         { return g_ladders; }
    Hell::SlotMap<PickUp>& GetPickUps()                         { return g_pickUps; }
    Hell::SlotMap<PictureFrame>& GetPictureFrames()             { return g_pictureFrames; }
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets()             { return g_powerPoleSets; }
    Hell::SlotMap<Staircase>& GetStaircases()                   { return g_staircases; }
    Hell::SlotMap<TrimSet>& GetTrimSets()                       { return g_trimSets; }
    Hell::SlotMap<Wall>& GetWalls()                             { return g_walls; }
    Hell::SlotMap<Window>& GetWindows()                         { return g_windows; }

    std::vector<BulletCasing>& GetBulletCasings()                       { return g_bulletCasings; }
    std::vector<ChristmasTree>& GetChristmasTrees()                     { return g_christmasTrees; }
    std::vector<Decal>& GetDecals()                                     { return g_newDecals; }
    std::vector<Dobermann>& GetDobermanns()                             { return g_dobermanns; }
    std::vector<GameObject>& GetGameObjects()                           { return g_gameObjects; }
    std::vector<Light>& GetLights()                                     { return g_lights; };
    std::vector<Kangaroo>& GetKangaroos()                               { return g_kangaroos; }
    std::vector<MapInstance>& GetMapInstances()                         { return g_mapInstances; }
    std::vector<Mermaid>& GetMermaids()                                 { return g_mermaids; }
    std::vector<Piano>& GetPianos()                                     { return g_pianos; }
    std::vector<SpawnPoint>& GetCampaignSpawnPoints()                   { return g_spawnCampaignPoints; }
    std::vector<SpawnPoint>& GetDeathmatchSpawnPoints()                 { return g_spawnDeathmatchPoints; }
    std::vector<Transform>& GetDoorAndWindowCubeTransforms()            { return g_doorAndWindowCubeTransforms; }
    std::vector<Road>& GetRoads()                                       { return g_roads; }
    std::vector<Shark>& GetSharks()                                     { return g_sharks; }
    //std::vector<Tree>& GetTrees()                                       { return g_trees; }
    std::vector<GPULight>& GetGPULightsLowRes()                 { return g_gpuLightsLowRes; }
    std::vector<GPULight>& GetGPULightsMidRes()                 { return g_gpuLightsMidRes; }
    std::vector<GPULight>& GetGPULightsHighRes()                { return g_gpuLightsHighRes; }

    std::vector<GPUAABB>& GetDirtyDoorAABBS()                   { return g_dirtyDoorAABBS; }
}
