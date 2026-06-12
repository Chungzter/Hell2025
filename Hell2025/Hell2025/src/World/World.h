#pragma once
#include <Hell/Types.h>
#include <Hell/SlotMap.h>

#include "Core/Debug.h"
#include "Types/Characters/Allies/Mermaid/Mermaid.h"
#include "Types/Characters/Enemies/Dobermann/Dobermann.h"
#include "Types/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Types/Characters/Enemies/Shark/Shark.h"
#include "Types/Christmas/ChristmasLights.h"
#include "Types/Christmas/ChristmasTree.h"
#include "Types/Core/GenericObject.h"
#include "Types/Effects/VolumetricBloodSplatter.h"
#include "Types/Effects/ScreenSpaceBloodDecal.h"
#include "Types/Exterior/Fence.h"
#include "Types/Exterior/Road.h"
#include "Types/Exterior/PowerPoleSet.h"
#include "Types/Exterior/Tree.h"
#include "Types/Game/AnimatedGameObject.h"
#include "Types/Game/Bullet.h"
#include "Types/Game/BulletCasing.h"
#include "Types/Game/Decal.h"
#include "Types/Game/GameObject.h"
#include "Types/Game/Ladder.h"
#include "Types/Game/Light.h"
#include "Types/Game/PickUp.h"
#include "Types/Game/Staircase.h"
#include "Types/Generics/GenericBouncable.h"
#include "Types/Generics/GenericStatic.h"
#include "Types/Interior/PictureFrame.h"
#include "Types/Interior/Piano.h"
#include "Types/House/Door.h"
#include "Types/House/Fireplace.h"
#include "Types/House/HouseInstance.h"
#include "Types/House/HousePlane.h"
#include "Types/House/TrimSet.h"
#include "Types/House/Wall.h"
#include "Types/House/Window.h"
#include "Types/Map/Map.h"
#include "Types/Map/MapInstance.h"
#include "Util/Util.h"
#include "glm/gtx/intersect.hpp"
#include <vector>
#include "Modelling/Clipping.h"

#include "Types/Renderer/MeshBuffer.h"

#include "GlobalIllumination/DDGIVolume.h" // move me to Types dir

struct MapInstanceCreateInfo {
    std::string mapName;
    uint32_t spawnOffsetChunkX;
    uint32_t spawnOffsetChunkZ;
};

namespace World {
    std::vector<SpriteSheetObject>& GetBubbleSpriteSheetObjects();

    void Init();
    void BeginFrame();
    void EndFrame();
    void Update(float deltaTime);

    void NewRun();

    void SubmitRenderItems();


    void ResetWorld();
    void ClearAllObjects();

    DDGIVolume& GetTestDDGIVolume();

    void LoadMapInstance(const std::string& mapName); // Calls the function below, but with a single map
    void LoadMapInstances(std::vector<MapInstanceCreateInfo> mapInstanceCreateInfoSet); // Calls the 3 functions below
    void LoadMapInstancesHeightMapData(std::vector<MapInstanceCreateInfo> mapInstanceCreateInfoSet);
    void LoadMapInstanceObjects(const std::string& mapName, SpawnOffset spawnOffset);
    void LoadMapInstanceHouses(const std::string& mapName, SpawnOffset spawnOffset);
    
    void LoadSingleHouse(const std::string& houseName);
    void LoadHouseInstance(const std::string& houseName, SpawnOffset spawnOffset);

    bool ChunkExists(int x, int z);
    const uint32_t GetChunkCountX();
    const uint32_t GetChunkCountZ();
    const uint32_t GetChunkCount(); 
    const HeightMapChunk* GetChunk(int x, int z);

    void AddDecal2(DecalCreateInfo createInfo);

    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());

    void AddBullet(BulletCreateInfo createInfo);
    void AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddBulletCasing(BulletCasingCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddCreateInfoCollection(CreateInfoCollection& createInfoCollection, SpawnOffset spawnOffset);
    void AddDobermann(DobermannCreateInfo& createInfo);
    void AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset);
    void AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset);
    void AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddHousePlane(HousePlaneCreateInfo createInfo, SpawnOffset spawnOffset);
    void AddKangaroo(const KangarooCreateInfo& createInfo);
    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    void AddScreenSpaceBloodDecal(ScreenSpaceBloodDecalCreateInfo createInfo);
    void AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset);
    void AddVATBlood(glm::vec3 position, glm::vec3 front);
    void AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset);

    void PrintObjectCounts();

    void EnableOcean();
    void DisableOcean();
    bool HasOcean();
    
    // Logic
    void ProcessBullets();
    void HackTest();

    // Creation
    void CreateGameObject();
    uint64_t CreateAnimatedGameObject();

    // Objects
    void SetObjectPosition(uint64_t objectId, const glm::vec3& position);
    void SetObjectRotation(uint64_t objectId, const glm::vec3& rotation);
    bool RemoveObject(uint64_t objectId);
    glm::vec3 GetGizmoOffest(uint64_t objectId);
    
    // BVH
	void UpdateBvhs();
    void MarkStaticSceneBvhDirty();
	BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);

    const float GetWorldSpaceWidth();
    const float GetWorldSpaceDepth();

    // Util
    bool ObjectTypeIsInteractable(ObjectType objectType, uint64_t objectId, glm::vec3 playerCameraPosition, glm::vec3 rayHitPosition);

    // Map
    const std::string& GetCurrentMapName();

    // House
    void RecreateClippingCubes();                     // you have this and the other one, they achieve the same thing, merge this logic
    void RecreateAllDoorAndWindowCubeTransforms();    // you have this and the other one, they achieve the same thing, merge this logic

    void RecreateAllHouseGeometry();
    void RecreateAllProceduralWallMesh();
    void RecreateAllProcedularHousePlaneMesh();
    void RecreateAllWeatherBoards();
    void RecreateAllWallTrims();
    void RecreateAllHangingLightCords();
    void RemoveAllWeatherBoards();

    // Spawns
    SpawnPoint GetRandomCampaignSpawnPoint();
    SpawnPoint GetRandomDeathmanSpawnPoint();
    void UpdateWorldSpawnPointsFromMap(Map* map);

    const glm::vec3& GetObjectPosition(uint64_t objectId);
    const glm::vec3& GetObjectRotation(uint64_t objectId);
    const std::string& GetObjectEditorName(uint64_t objectId);

    AnimatedGameObject* GetAnimatedGameObjectByObjectId(uint64_t objectId);
    CreateInfoCollection GetCreateInfoCollection();
    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t id, int32_t meshNodeLocalIndex);

    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId);
    DDGIVolume* GetDDGIVolumeByObjectId(uint64_t objectId);
    Door* GetDoorByObjectId(uint64_t objectId);
    Fence* GetFenceByObjectId(uint64_t objectId);
    Fireplace* GetFireplaceById(uint64_t objectId);
    GenericObject* GetGenericObjectById(uint64_t objectId);
    HousePlane* GetHousePlaneByObjectId(uint64_t objectId);
    Ladder* GetLadderByObjectId(uint64_t objectId);
    Light* GetLightByObjectId(uint64_t objectId); // Does not use slot map
    PowerPoleSet* GetPowerPoleSetByObjectId(uint64_t objectId);
    Staircase* GetStaircaseByObjectId(uint64_t objectId);

    Piano* GetPianoByObjectId(uint64_t objectId);
    Piano* GetPianoByMeshNodeObjectId(uint64_t objectId);
    PianoKey* GetPianoKeyByObjectId(uint64_t objectId);
    PickUp* GetPickUpByObjectId(uint64_t objectID);
    PictureFrame* GetPictureFrameByObjectId(uint64_t objectId);
    Wall* GetWallByObjectId(uint64_t objectId);
    Wall* GetWallByWallSegmentObjectId(uint64_t objectId);
    Shark* GetSharkByObjectId(uint64_t objectId);
    Window* GetWindowByObjectId(uint64_t objectId);
    GameObject* GetGameObjectByIndex(int32_t index);
    GameObject* GetGameObjectByName(const std::string& name);
    Light* GetLightByIndex(int32_t index);

    uint32_t GetLightCount();

    Hell::SlotMap<AnimatedGameObject>& GetAnimatedGameObjects();
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets();
    Hell::SlotMap<DDGIVolume>& GetDDGIVolumes();
    Hell::SlotMap<Door>& GetDoors();
    Hell::SlotMap<Fence>& GetFences();
    Hell::SlotMap<Fireplace>& GetFireplaces();
    Hell::SlotMap<GenericObject>& GetGenericObjects();
    Hell::SlotMap<HousePlane>& GetHousePlanes();
    Hell::SlotMap<Ladder>& GetLadders();
    Hell::SlotMap<PickUp>& GetPickUps();
    Hell::SlotMap<PictureFrame>& GetPictureFrames();
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets();
    Hell::SlotMap<Staircase>& GetStaircases();
    Hell::SlotMap<TrimSet>& GetTrimSets();
    Hell::SlotMap<Wall>& GetWalls();
    Hell::SlotMap<Window>& GetWindows();

    std::vector<ScreenSpaceBloodDecal>& GetScreenSpaceBloodDecals();
    std::vector<Bullet>& GetBullets();
    std::vector<BulletCasing>& GetBulletCasings();
    std::vector<ChristmasTree>& GetChristmasTrees();
    std::vector<ClippingCube>& GetClippingCubes();
    std::vector<Decal>& GetDecals();
    std::vector<Dobermann>& GetDobermanns();
    std::vector<GameObject>& GetGameObjects();
    std::vector<HeightMapChunk>& GetHeightMapChunks();
    std::vector<Light>& GetLights();
    std::vector<Kangaroo>& GetKangaroos();
    std::vector<MapInstance>& GetMapInstances();
    std::vector<Mermaid>& GetMermaids();
    std::vector<Piano>& GetPianos();
    std::vector<SpawnPoint>& GetCampaignSpawnPoints();
    std::vector<SpawnPoint>& GetDeathmatchSpawnPoints();
    std::vector<Transform>& GetDoorAndWindowCubeTransforms();
    std::vector<Road>& GetRoads();
    std::vector<Shark>& GetSharks();
    std::vector<VolumetricBloodSplatter>& GetVolumetricBloodSplatters();

    std::vector<uint64_t> GetLightIds();
    std::vector<GPUAABB>& GetDirtyDoorAABBS();
}