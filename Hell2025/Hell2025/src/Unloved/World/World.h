#pragma once

#include "Hell/Containers/SlotMap.h"
#include "Unloved/Common/CreateInfo.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct GPULight;

namespace Unloved {
    struct AnimatedGameObject;
    struct BulletCasing;
    struct ChristmasLightSet;
    struct ChristmasTree;
    struct DDGIVolume;
    struct Decal;
    struct Dobermann;
    struct Door;
    struct Fence;
    struct Fireplace;
    struct GameObject;
    struct GenericObject;
    struct WorldPlane;
    struct Kangaroo;
    struct Ladder;
    struct Light;
    struct MapInstance;
    struct Mermaid;
    struct Piano;
    struct PianoKey;
    struct PickUp;
    struct PictureFrame;
    struct PowerPoleSet;
    struct Road;
    struct Shark;
    struct SpawnPoint;
    struct SpriteSheetObject;
    struct Staircase;
    struct Terrain;
    struct TerrainChunk;
    struct TrimSet;
    struct Wall;
    struct Window;

}

namespace Unloved::World {
    void Init();
    void NewRun();
    void BeginFrame();
    void UpdateBvhs();
    void Update();
    void UpdatePlayers();
    void UpdateLegacyObjects();
    void SubmitRenderItems();
    void EndFrame();
    void CleanUp();

    void ResetWorld();
    void ClearAllObjects();

    void UpdateEnvironment();
    const glm::vec3& GetMoonlightDirection();

    CreateInfoCollection GetCreateInfoCollection();
    void AddCreateInfoCollection(CreateInfoCollection& createInfoCollection, SpawnOffset spawnOffset);

    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddBulletCasing(BulletCasingCreateInfo createInfo);
    uint64_t AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDobermann(DobermannCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddHousePlane(HousePlaneCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddKangaroo(KangarooCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());

    AnimatedGameObject* GetAnimatedGameObjectByObjectId(uint64_t objectId);
    BulletCasing* GetBulletCasingByObjectId(uint64_t objectId);
    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId);
    ChristmasTree* GetChristmasTreeByObjectId(uint64_t objectId);
    DDGIVolume* GetDDGIVolumeByObjectId(uint64_t objectId);
    Dobermann* GetDobermannByObjectId(uint64_t objectId);
    Door* GetDoorByObjectId(uint64_t objectId);
    Fence* GetFenceByObjectId(uint64_t objectId);
    Fireplace* GetFireplaceById(uint64_t objectId);
    GameObject* GetGameObjectByObjectId(uint64_t objectId);
    GameObject* GetGameObjectByIndex(int32_t index);
    GameObject* GetGameObjectByName(const std::string& name);
    GenericObject* GetGenericObjectById(uint64_t objectId);
    WorldPlane* GetHousePlaneByObjectId(uint64_t objectId);
    Kangaroo* GetKangarooByObjectId(uint64_t objectId);
    Ladder* GetLadderByObjectId(uint64_t objectId);
    Light* GetLightByObjectId(uint64_t objectId);
    Light* GetLightByIndex(int32_t index);
    uint32_t GetLightCount();
    std::vector<uint64_t> GetLightIds();
    Mermaid* GetMermaidByObjectId(uint64_t objectId);
    Piano* GetPianoByObjectId(uint64_t objectId);
    Piano* GetPianoByMeshNodeObjectId(uint64_t objectId);
    PianoKey* GetPianoKeyByObjectId(uint64_t objectId);
    PickUp* GetPickUpByObjectId(uint64_t objectId);
    PictureFrame* GetPictureFrameByObjectId(uint64_t objectId);
    PowerPoleSet* GetPowerPoleSetByObjectId(uint64_t objectId);
    Staircase* GetStaircaseByObjectId(uint64_t objectId);
    TrimSet* GetTrimSetByObjectId(uint64_t objectId);
    Wall* GetWallByObjectId(uint64_t objectId);
    Wall* GetWallByWallSegmentObjectId(uint64_t objectId);
    Window* GetWindowByObjectId(uint64_t objectId);

    Hell::SlotMap<AnimatedGameObject>& GetAnimatedGameObjects();
    Hell::SlotMap<BulletCasing>& GetBulletCasings();
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets();
    Hell::SlotMap<ChristmasTree>& GetChristmasTrees();
    Hell::SlotMap<DDGIVolume>& GetDDGIVolumes();
    Hell::SlotMap<Decal>& GetDecals();
    Hell::SlotMap<Dobermann>& GetDobermanns();
    Hell::SlotMap<Door>& GetDoors();
    Hell::SlotMap<Fence>& GetFences();
    Hell::SlotMap<Fireplace>& GetFireplaces();
    Hell::SlotMap<GameObject>& GetGameObjects();
    Hell::SlotMap<GenericObject>& GetGenericObjects();
    Hell::SlotMap<GPULight>& GetGPULightsLowRes();
    Hell::SlotMap<GPULight>& GetGPULightsMidRes();
    Hell::SlotMap<GPULight>& GetGPULightsHighRes();
    Hell::SlotMap<WorldPlane>& GetWorldPlanes();
    Hell::SlotMap<Kangaroo>& GetKangaroos();
    Hell::SlotMap<Ladder>& GetLadders();
    Hell::SlotMap<Light>& GetLights();
    Hell::SlotMap<MapInstance>& GetMapInstances();
    Hell::SlotMap<Mermaid>& GetMermaids();
    Hell::SlotMap<Piano>& GetPianos();
    Hell::SlotMap<PickUp>& GetPickUps();
    Hell::SlotMap<PictureFrame>& GetPictureFrames();
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets();
    Hell::SlotMap<Road>& GetRoads();
    Hell::SlotMap<Shark>& GetSharks();
    Hell::SlotMap<SpawnPoint>& GetCampaignSpawnPoints();
    Hell::SlotMap<SpawnPoint>& GetDeathmatchSpawnPoints();
    Hell::SlotMap<SpriteSheetObject>& GetBubbleSpriteSheetObjects();
    Hell::SlotMap<Staircase>& GetStaircases();
    Hell::SlotMap<TerrainChunk>& GetTerrainChunks();
    Hell::SlotMap<TrimSet>& GetTrimSets();
    Hell::SlotMap<Wall>& GetWalls();
    Hell::SlotMap<Window>& GetWindows();
}
