#include "LegacyWorld.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Time.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
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
#include "Unloved/World/World.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Types.h"
#include "Util.h"

using namespace Hell;

namespace Unloved::LegacyWorld {

    std::vector<Decal> g_newDecals;
    std::vector<HeightMapChunk> g_heightMapChunks;
    std::vector<MapInstance> g_mapInstances;
    std::vector<Road> g_roads;
    std::vector<Shark> g_sharks;
    std::vector<SpawnPoint> g_spawnCampaignPoints;
    std::vector<SpawnPoint> g_spawnDeathmatchPoints;
    std::vector<Transform> g_doorAndWindowCubeTransforms;
    //std::vector<Tree> g_trees;

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
        Unloved::World::AddGameObject(createInfo);
        Unloved::World::GetGameObjects()[0].SetMeshMaterial("ReflectorPole", "Fence");
        Unloved::World::GetGameObjects()[0].SetMeshMaterial("ReflectorRed", "Red");

        DobermannCreateInfo dobermannCreateInfo;
        dobermannCreateInfo.position = glm::vec3(37.2f, 31.0f, 35.3f);
        Unloved::World::AddDobermann(dobermannCreateInfo);

        KangarooCreateInfo kangarooCreateInfo;
        kangarooCreateInfo.position = glm::vec3(48, 32.6, 39);
        kangarooCreateInfo.rotation = glm::vec3(0, HELL_PI * -0.5f, 0);
        Unloved::World::AddKangaroo(kangarooCreateInfo);

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
        Unloved::World::AddCreateInfoCollection(createInfoCollection, spawnOffset);


        MermaidCreateInfo mermaidCreateInfo;

        mermaidCreateInfo.position = glm::vec3(14.0f, 29.0f, 36.5f); // outdoors

        mermaidCreateInfo.position = glm::vec3(36.0f, 31.0f, 36.5f); // indoors
        mermaidCreateInfo.rotation.y = 0.05f;                        // indoors

        Unloved::World::AddMermaid(mermaidCreateInfo);


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
        Unloved::World::AddCreateInfoCollection(map->GetCreateInfoCollection(), spawnOffset);

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
        for (Kangaroo& kangaroo : Unloved::World::GetKangaroos()) {
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


    const glm::vec3& GetObjectPosition(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);

        if (DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(objectId)) return object->GetOrigin();
        // etc

        Logging::Warning() << "LegacyWorld::GetObjectPosition(..) failed for ID " << objectId << ". You haven't implemented " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n";
        return invalid;
    }

    const glm::vec3& GetObjectRotation(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);

        if (DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(objectId)) return object->GetRotation();
        // etc

        Logging::Warning() << "LegacyWorld::GetObjectRotation(..) failed for ID " << objectId << ". You haven't implemented " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n";
        return invalid;
    }

    const std::string& GetObjectEditorName(uint64_t objectId) {
        const static std::string invalid = UNDEFINED_STRING;

        if (DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(objectId)) return object->GetEditorName();
        if (Door* object = Unloved::World::GetDoorByObjectId(objectId))             return object->GetEditorName();
        // etc

        return invalid;
    }

    uint64_t CreateAnimatedGameObject() {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::ANIMATED_GAME_OBJECT);
        Unloved::World::GetAnimatedGameObjects().emplace_with_id(id, id);
        return id;
    }

    //Tree* GetTreeByIndex(int32_t index) {
    //    if (index >= 0 && index < g_trees.size()) {
    //        return &g_trees[index];
    //    }
    //    else {
    //        return nullptr;
    //    }
    //}

    void SetObjectPosition(uint64_t objectId, const glm::vec3& position) {

        if (DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(objectId)) object->SetOrigin(position);

        if (Door* door = Unloved::World::GetDoorByObjectId(objectId)) {
            door->SetPosition(position);
            RecreateAllHouseGeometry();
            Hell::Physics::ForceZeroStepUpdate();
        }

        if (GenericObject* genericObject = Unloved::World::GetGenericObjectById(objectId)) {
            genericObject->SetPosition(position);
        }

        if (Fireplace* fireplace= Unloved::World::GetFireplaceById(objectId)) {
            fireplace->SetPosition(position);
        }

        if (Piano* piano = Unloved::World::GetPianoByObjectId(objectId)) {
            piano->SetPosition(position);
            Hell::Physics::ForceZeroStepUpdate();
        }

        if (WorldPlane* plane = Unloved::World::GetHousePlaneByObjectId(objectId)) {
            plane->UpdateWorldSpaceCenter(position);
            RecreateAllHouseGeometry();
        }

        if (Ladder* ladder = Unloved::World::GetLadderByObjectId(objectId)) {
            ladder->SetPosition(position);
        }

        if (Light* light = Unloved::World::GetLightByObjectId(objectId)) {
            light->SetPosition(position);
        }

        if (PickUp* pickUp = Unloved::World::GetPickUpByObjectId(objectId)) {
            pickUp->SetPosition(position);
        }

        if (PictureFrame* pictureFrame = Unloved::World::GetPictureFrameByObjectId(objectId)) {
            pictureFrame->SetPosition(position);
        }

        if (Staircase* staircase = Unloved::World::GetStaircaseByObjectId(objectId)) {
            staircase->SetPosition(position);
        }

        //if (Tree* tree = LegacyWorld::GetTreeByObjectId(objectId)) {
        //    tree->SetPosition(position);
        //}

        if (Wall* wall = Unloved::World::GetWallByObjectId(objectId)) {
            wall->UpdateWorldSpaceCenter(position);
            Hell::Physics::ForceZeroStepUpdate();
            RecreateAllHouseGeometry();
        }

        if (Window* window = Unloved::World::GetWindowByObjectId(objectId)) {
            window->SetPosition(position);
            RecreateAllHouseGeometry();
            Hell::Physics::ForceZeroStepUpdate();
        }
    }

    void SetObjectRotation(uint64_t objectId, const glm::vec3& rotation) {
        if (DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(objectId)) object->SetRotation(rotation);

        if (Fireplace* object = Unloved::World::GetFireplaceById(objectId)) {
            object->SetRotation(rotation);
        }
        if (GenericObject* object = Unloved::World::GetGenericObjectById(objectId)) {
            object->SetRotation(rotation);
        }
        if (Ladder* object = Unloved::World::GetLadderByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
        if (PickUp* object = Unloved::World::GetPickUpByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
        if (Staircase* object = Unloved::World::GetStaircaseByObjectId(objectId)) {
            object->SetRotation(rotation);
        }
    }

    glm::vec3 GetGizmoOffest(uint64_t objectId) {
        GenericObject* drawers = Unloved::World::GetGenericObjectById(objectId);
        if (drawers) {
            return drawers->GetGizmoOffset();
        }
        return glm::vec3(0.0f);
    }

    bool RemoveObject(uint64_t objectId) {
        if (objectId == 0) return false;

        if (Unloved::World::GetAnimatedGameObjects().contains(objectId)) {
            Unloved::World::GetAnimatedGameObjects().get(objectId)->CleanUp();
            Unloved::World::GetAnimatedGameObjects().erase(objectId);
            return true;
        }

        if (Unloved::BulletSystem::RemoveBulletTrail(objectId)) {
            return true;
        }

        if (Unloved::World::GetChristmasLightSets().contains(objectId)) {
            Unloved::World::GetChristmasLightSets().get(objectId)->CleanUp();
            Unloved::World::GetChristmasLightSets().erase(objectId);
            return true;
        }

        if (Unloved::World::GetChristmasTrees().contains(objectId)) {
            Unloved::World::GetChristmasTrees().get(objectId)->CleanUp();
            Unloved::World::GetChristmasTrees().erase(objectId);
            return true;
        }

        if (Unloved::World::GetDobermanns().contains(objectId)) {
            Unloved::World::GetDobermanns().get(objectId)->CleanUp();
            Unloved::World::GetDobermanns().erase(objectId);
            return true;
        }

        if (Unloved::World::GetDoors().contains(objectId)) {
            Unloved::World::GetDoors().get(objectId)->CleanUp();
            Unloved::World::GetDoors().erase(objectId);
            return true;
        }

        if (Unloved::World::GetFences().contains(objectId)) {
            Unloved::World::GetFences().get(objectId)->CleanUp();
            Unloved::World::GetFences().erase(objectId);
            return true;
        }

        if (Unloved::World::GetFireplaces().contains(objectId)) {
            Unloved::World::GetFireplaces().get(objectId)->CleanUp();
            Unloved::World::GetFireplaces().erase(objectId);
            return true;
        }

        if (Unloved::World::GetGenericObjects().contains(objectId)) {
            Unloved::World::GetGenericObjects().get(objectId)->CleanUp();
            Unloved::World::GetGenericObjects().erase(objectId);
            return true;
        }

        if (Unloved::World::GetGameObjects().contains(objectId)) {
            Unloved::World::GetGameObjects().get(objectId)->CleanUp();
            Unloved::World::GetGameObjects().erase(objectId);
            return true;
        }

        if (Unloved::World::GetWorldPlanes().contains(objectId)) {
            Unloved::World::GetWorldPlanes().get(objectId)->CleanUp();
            Unloved::World::GetWorldPlanes().erase(objectId);
            return true;
        }

        if (Unloved::World::GetKangaroos().contains(objectId)) {
            Unloved::World::GetKangaroos().get(objectId)->CleanUp();
            Unloved::World::GetKangaroos().erase(objectId);
            return true;
        }

        if (Unloved::World::GetLights().contains(objectId)) {
            Unloved::World::GetLights().erase(objectId);
            return true;
        }

        if (Unloved::World::GetMermaids().contains(objectId)) {
            Unloved::World::GetMermaids().get(objectId)->CleanUp();
            Unloved::World::GetMermaids().erase(objectId);
            return true;
        }

        if (Unloved::World::GetPowerPoleSets().contains(objectId)) {
            Unloved::World::GetPowerPoleSets().get(objectId)->CleanUp();
            Unloved::World::GetPowerPoleSets().erase(objectId);
            return true;
        }

        if (Unloved::World::GetStaircases().contains(objectId)) {
            Unloved::World::GetStaircases().get(objectId)->CleanUp();
            Unloved::World::GetStaircases().erase(objectId);
            return true;
        }

        if (Unloved::World::GetTrimSets().contains(objectId)) {
            Unloved::World::GetTrimSets().get(objectId)->CleanUp();
            Unloved::World::GetTrimSets().erase(objectId);
            return true;
        }

        if (Unloved::World::GetPickUps().contains(objectId)) {
            // Dirty any lights within range... maybe put this somewhere else
            PickUp* pickUp = Unloved::World::GetPickUpByObjectId(objectId);

            for (Light& light : Unloved::World::GetLights()) {
                if (pickUp->GetMeshNodes().m_worldspaceAABB.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                    light.ForceDirty();
                }
            }

            Unloved::World::GetPickUps().get(objectId)->CleanUp();
            Unloved::World::GetPickUps().erase(objectId);
            return true;
        }

        if (Unloved::World::GetPictureFrames().contains(objectId)) {
            Unloved::World::GetPictureFrames().get(objectId)->CleanUp();
            Unloved::World::GetPictureFrames().erase(objectId);
            return true;
        }

        if (Unloved::World::GetLadders().contains(objectId)) {
            Unloved::World::GetLadders().get(objectId)->CleanUp();
            Unloved::World::GetLadders().erase(objectId);
            return true;
        }

        if (Unloved::World::GetWalls().contains(objectId)) {
            Unloved::World::GetWalls().get(objectId)->CleanUp();
            Unloved::World::GetWalls().erase(objectId);
            return true;
        }

        if (Unloved::World::GetWindows().contains(objectId)) {
            Unloved::World::GetWindows().get(objectId)->CleanUp();
            Unloved::World::GetWindows().erase(objectId);
            return true;
        }

        if (Unloved::World::GetPianos().contains(objectId)) {
            Unloved::World::GetPianos().get(objectId)->CleanUp();
            Unloved::World::GetPianos().erase(objectId);
            return true;
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

        Unloved::World::GetGameObjects().clear();

        GameObjectCreateInfo createInfo2;
        createInfo2.position = glm::vec3(49.0f, 31.0f, 39.0f);
        createInfo2.scale = glm::vec3(1.0f);
        createInfo2.modelName = "Bunny";
        Unloved::World::AddGameObject(createInfo2);
        Unloved::World::GetGameObjects()[0].m_meshNodes.SetMeshMaterials("Leopard");
        Unloved::World::GetGameObjects()[0].SetPosition(glm::vec3(39.0f, 31.0f, 39.0f));
    }

    void ClearAllObjects() {
        RemoveAllWeatherBoards();
        Unloved::MirrorManager::CleanUp();
        Unloved::BulletSystem::CleanUp();
        Unloved::BloodSystem::CleanUp();
        Ocean::DestroyPhysicsPlane();

        for (BulletCasing& bulletCasing : Unloved::World::GetBulletCasings()) bulletCasing.CleanUp();
        for (ChristmasLightSet& christmasLights : Unloved::World::GetChristmasLightSets()) christmasLights.CleanUp();
        for (ChristmasTree& christmasTree : Unloved::World::GetChristmasTrees()) christmasTree.CleanUp();
        for (DDGIVolume& object : Unloved::World::GetDDGIVolumes())             object.CleanUp();
        for (Door& door : Unloved::World::GetDoors())                            door.CleanUp();
        for (Fireplace& fireplace : Unloved::World::GetFireplaces())             fireplace.CleanUp();
        for (GenericObject& drawer : Unloved::World::GetGenericObjects())        drawer.CleanUp();
        for (Fence& fence : Unloved::World::GetFences())                         fence.CleanUp();
        for (GameObject& gameObject : Unloved::World::GetGameObjects())          gameObject.CleanUp();
        //for (Kangaroo& kangaroo : Unloved::World::GetKangaroos())              kangaroo.CleanUp();
        for (Ladder& ladder : Unloved::World::GetLadders())                      ladder.CleanUp();
        for (Mermaid& mermaid : Unloved::World::GetMermaids())                   mermaid.CleanUp();
        for (WorldPlane& housePlane : Unloved::World::GetWorldPlanes())          housePlane.CleanUp();
        for (Piano& piano : Unloved::World::GetPianos())                         piano.CleanUp();
        for (PickUp& pickUp : Unloved::World::GetPickUps())                      pickUp.CleanUp();
        for (PowerPoleSet& powerPoleSet : Unloved::World::GetPowerPoleSets())    powerPoleSet.CleanUp();
        for (Shark& shark : g_sharks)                                   shark.CleanUp();
        for (Staircase& staircase: Unloved::World::GetStaircases())              staircase.CleanUp();
        for (SpawnPoint& spawnPoint : g_spawnCampaignPoints)            spawnPoint.CleanUp();
        for (SpawnPoint& spawnPoint : g_spawnDeathmatchPoints)          spawnPoint.CleanUp();
        //for (Tree& tree : g_trees)                                      tree.CleanUp();
        for (TrimSet& trimSet : Unloved::World::GetTrimSets())                   trimSet.CleanUp();
        for (Wall& wall : Unloved::World::GetWalls())                            wall.CleanUp();
        for (Window& window : Unloved::World::GetWindows())                      window.CleanUp();

        //for (auto& [id, drawers] : g_drawers) drawers.CleanUp();

        // Clear all containers
        Unloved::World::GetBulletCasings().clear();
        Unloved::World::GetChristmasLightSets().clear();
        Unloved::World::GetChristmasTrees().clear();
        Unloved::World::GetDDGIVolumes().clear();
        Unloved::World::GetDoors().clear();
        Unloved::World::GetFireplaces().clear();
        Unloved::World::GetGenericObjects().clear();
        Unloved::World::GetFences().clear();
        Unloved::World::GetGameObjects().clear();
        //Unloved::World::GetKangaroos().clear();
        Unloved::World::GetLadders().clear();
        Unloved::World::GetLights().clear();
        Unloved::World::GetMermaids().clear();
        Unloved::World::GetPianos().clear();
        Unloved::World::GetPickUps().clear();
        Unloved::World::GetWorldPlanes().clear();
        Unloved::World::GetPictureFrames().clear();
        Unloved::World::GetPowerPoleSets().clear();
        g_sharks.clear();
        g_spawnCampaignPoints.clear();
        g_spawnDeathmatchPoints.clear();
        //g_trees.clear();
        Unloved::World::GetTrimSets().clear();
        Unloved::World::GetWalls().clear();
        Unloved::World::GetWindows().clear();
        Unloved::World::GetStaircases().clear();
    }

    void AddDecal2(DecalCreateInfo createInfo) {
        g_newDecals.push_back(Decal(createInfo));
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
            int rand = Hell::Random::Int(0, g_spawnCampaignPoints.size() - 1); g_spawnCampaignPoints[rand];
            spawnPoint = g_spawnCampaignPoints[rand];
        }
        else {
            int rand = Hell::Random::Int(0, g_fallbackSpawnPoints.size() - 1); g_fallbackSpawnPoints[rand];
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
            int rand = Hell::Random::Int(0, g_spawnDeathmatchPoints.size() - 1); g_spawnDeathmatchPoints[rand];
            spawnPoint = g_spawnDeathmatchPoints[rand];
        }
        else {
            int rand = Hell::Random::Int(0, g_fallbackSpawnPoints.size() - 1); g_fallbackSpawnPoints[rand];
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

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t id, int32_t meshNodeLocalIndex) {
        if (meshNodeLocalIndex < 0) return nullptr;

        if (Door* object = Unloved::World::GetDoorByObjectId(id))               return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Fireplace* object = Unloved::World::GetFireplaceById(id))           return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (GenericObject* object = Unloved::World::GetGenericObjectById(id))   return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Piano* object = Unloved::World::GetPianoByObjectId(id))             return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
        if (Window* object = Unloved::World::GetWindowByObjectId(id))           return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);

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

    Shark* GetSharkByObjectId(uint64_t objectId) {
        for (Shark& shark: g_sharks) {
            if (shark.GetObjectId() == objectId) {
                return &shark;
            }
        }
        return nullptr;
    }

    std::vector<Decal>& GetDecals()                                     { return g_newDecals; }
    std::vector<MapInstance>& GetMapInstances()                         { return g_mapInstances; }
    std::vector<SpawnPoint>& GetCampaignSpawnPoints()                   { return g_spawnCampaignPoints; }
    std::vector<SpawnPoint>& GetDeathmatchSpawnPoints()                 { return g_spawnDeathmatchPoints; }
    std::vector<Transform>& GetDoorAndWindowCubeTransforms()            { return g_doorAndWindowCubeTransforms; }
    std::vector<Road>& GetRoads()                                       { return g_roads; }
    std::vector<Shark>& GetSharks()                                     { return g_sharks; }
    //std::vector<Tree>& GetTrees()                                       { return g_trees; }
    std::vector<GPULight>& GetGPULightsLowRes()                 { return g_gpuLightsLowRes; }
    std::vector<GPULight>& GetGPULightsMidRes()                 { return g_gpuLightsMidRes; }
    std::vector<GPULight>& GetGPULightsHighRes()                { return g_gpuLightsHighRes; }

}
