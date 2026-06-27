#pragma once

#include "Hell/Containers/SlotMap.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Unloved {
    struct AnimatedGameObject;
    struct BulletCasing;
    struct ChristmasLightSet;
    struct ChristmasTree;
    struct ClippingCube;
    struct DDGIVolume;
    struct Decal;
    struct Dobermann;
    struct Door;
    struct Fence;
    struct Fireplace;
    struct GameObject;
    struct GenericObject;
    struct GPULight;
    struct GPUAABB;
    struct HousePlane;
    struct Kangaroo;
    struct Ladder;
    struct Light;
    struct MapInstance;
    struct Mermaid;
    struct Piano;
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
    struct Transform;
    struct TrimSet;
    struct Wall;
    struct Window;

    struct MapInstanceCreateInfo {
        std::string mapName;
        uint32_t spawnOffsetChunkX = 0;
        uint32_t spawnOffsetChunkZ = 0;
    };
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

    Hell::SlotMap<AnimatedGameObject>& GetAnimatedGameObjects();
    Hell::SlotMap<BulletCasing>& GetBulletCasings();
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets();
    Hell::SlotMap<ChristmasTree>& GetChristmasTrees();
    Hell::SlotMap<ClippingCube>& GetClippingCubes();
    Hell::SlotMap<Unloved::DDGIVolume>& GetDDGIVolumes();
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
    Hell::SlotMap<GPUAABB>& GetDirtyDoorAABBS();
    Hell::SlotMap<HousePlane>& GetHousePlanes();
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
    Hell::SlotMap<Transform>& GetDoorAndWindowCubeTransforms();
    Hell::SlotMap<TrimSet>& GetTrimSets();
    Hell::SlotMap<Wall>& GetWalls();
    Hell::SlotMap<Window>& GetWindows();
}
