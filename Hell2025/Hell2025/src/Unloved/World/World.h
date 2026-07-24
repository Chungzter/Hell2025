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
    struct Jetty;
    struct Kangaroo;
    struct Ladder;
    struct Light;
    struct Map;
    struct MapCreateInfo;
    struct MapData;
    struct Mermaid;
    struct MeshNode;
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
    struct Wire;
    struct WireCreateInfo;
    struct Window;
    struct HouseData;
    struct WorldPlane;
}

namespace Unloved::World {
    void Init();
    void NewRun();
    void BeginFrame();
    void UpdateBvhs();
    void Update();
    void UpdateEnemyMovement();
    void UpdateObjects();
    void UpdatePlayers();
    void SubmitRenderItems();
    void EndFrame();
    void CleanUp();

    void CleanUpAll();
    void CleanUpCasings();
    void CleanUpDecals();

    void ResetWorld();
    void ClearAllObjects();

    bool HasLoadedMap();
    bool HasOcean();
    void RefreshOceanPhysics();

    void UpdateEnvironment();
    const glm::vec3& GetMoonlightDirection();

    CreateInfoCollection GetCreateInfoCollection();
    void AddCreateInfoCollection(const CreateInfoCollection& createInfoCollection, SpawnOffset spawnOffset);

    void LoadMap(const std::string& mapName);
    void LoadMaps(const std::vector<MapCreateInfo>& mapCreateInfoSet);
    void LoadMap(const MapData& mapData, SpawnOffset spawnOffset);
    void LoadMapObjects(const MapData& mapData, SpawnOffset spawnOffset);
    void LoadMapHouses(const MapData& mapData, SpawnOffset spawnOffset);
    void LoadSingleHouse(const std::string& houseName);
    void LoadHouse(const std::string& houseName, SpawnOffset spawnOffset);
    void LoadHouse(const HouseData& houseData, SpawnOffset spawnOffset);

    bool RemoveObjectById(uint64_t objectId);
    bool SetPositionById(uint64_t objectId, const glm::vec3& position);
    bool SetRotationById(uint64_t objectId, const glm::vec3& rotation);

    const glm::vec3& GetPositionById(uint64_t objectId);
    const glm::vec3& GetRotationById(uint64_t objectId);
    const std::string& GetEditorNameById(uint64_t objectId);

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t objectId, int32_t meshNodeLocalIndex);

    uint64_t AddAnimatedGameObject();
    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddBulletCasing(BulletCasingCreateInfo createInfo);
    uint64_t AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDecal(DecalCreateInfo createInfo);
    uint64_t AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDobermann(DobermannCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWorldPlane(WorldPlaneCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddKangaroo(KangarooCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddJetty(JettyCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddShark(SharkCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddSpawnPointCampaign(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddSpawnPointDeathMatch(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    uint64_t AddWire(WireCreateInfo createInfo);
    uint64_t AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());

    AnimatedGameObject* GetAnimatedGameObjectByObjectId(uint64_t objectId);
    BulletCasing* GetBulletCasingByObjectId(uint64_t objectId);
    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId);
    ChristmasTree* GetChristmasTreeByObjectId(uint64_t objectId);
    Decal* GetDecalByObjectId(uint64_t objectId);
    DDGIVolume* GetDDGIVolumeByObjectId(uint64_t objectId);
    Dobermann* GetDobermannByObjectId(uint64_t objectId);
    Door* GetDoorByObjectId(uint64_t objectId);
    Fence* GetFenceByObjectId(uint64_t objectId);
    Fireplace* GetFireplaceById(uint64_t objectId);
    GameObject* GetGameObjectByObjectId(uint64_t objectId);
    GameObject* GetGameObjectByIndex(int32_t index);
    GameObject* GetGameObjectByName(const std::string& name);
    GenericObject* GetGenericObjectById(uint64_t objectId);
    WorldPlane* GetWorldPlaneByObjectId(uint64_t objectId);
    Kangaroo* GetKangarooByObjectId(uint64_t objectId);
    Jetty* GetJettyById(uint64_t objectId);
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
    Shark* GetSharkByObjectId(uint64_t objectId);
    SpawnPoint* GetSpawnPointCampaignByObjectId(uint64_t objectId);
    SpawnPoint* GetSpawnPointDeathMatchByObjectId(uint64_t objectId);
    Staircase* GetStaircaseByObjectId(uint64_t objectId);
    TrimSet* GetTrimSetByObjectId(uint64_t objectId);
    Wall* GetWallByObjectId(uint64_t objectId);
    Wall* GetWallByWallSegmentObjectId(uint64_t objectId);
    Wire* GetWireByObjectId(uint64_t objectId);
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
    Hell::SlotMap<WorldPlane>& GetWorldPlanes();
    Hell::SlotMap<Kangaroo>& GetKangaroos();
    Hell::SlotMap<Jetty>& GetJetties();
    Hell::SlotMap<Ladder>& GetLadders();
    Hell::SlotMap<Light>& GetLights();
    std::vector<Map>& GetMaps();
    Hell::SlotMap<Mermaid>& GetMermaids();
    Hell::SlotMap<Piano>& GetPianos();
    Hell::SlotMap<PickUp>& GetPickUps();
    Hell::SlotMap<PictureFrame>& GetPictureFrames();
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets();
    Hell::SlotMap<Road>& GetRoads();
    Hell::SlotMap<Shark>& GetSharks();
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsCampaign();
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsDeathMatch();
    Hell::SlotMap<SpriteSheetObject>& GetBubbleSpriteSheetObjects();
    Hell::SlotMap<Staircase>& GetStaircases();
    Hell::SlotMap<TerrainChunk>& GetTerrainChunks();
    Hell::SlotMap<TrimSet>& GetTrimSets();
    Hell::SlotMap<Wall>& GetWalls();
    Hell::SlotMap<Wire>& GetWires();
    Hell::SlotMap<Window>& GetWindows();
}
