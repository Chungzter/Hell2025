#pragma once
#include "Unloved/Common/Types.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Math/Transform.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Tree.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Objects/Props/GenericBouncable.h"
#include "Unloved/Objects/Props/GenericStatic.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/HouseInstance.h"
#include "Unloved/Objects/House/HousePlane.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Renderables/MeshBufferOLD.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

#include <vector>

// get me out of here
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapInstance.h"
#include "Util/Util.h"
//

struct MapInstanceCreateInfo {
    std::string mapName;
    uint32_t spawnOffsetChunkX;
    uint32_t spawnOffsetChunkZ;
};

struct HouseOccluderTriangle {
    glm::vec3 v0 = glm::vec3(0.0f);
    glm::vec3 v1 = glm::vec3(0.0f);
    glm::vec3 v2 = glm::vec3(0.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec2 uv1 = glm::vec2(0.0f);
    glm::vec2 uv2 = glm::vec2(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    int baseColorTextureIndex = -1;
    int rmaTextureIndex = -1;
};

namespace Unloved::LegacyWorld {

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

    void PrintObjectCounts();

    void EnableOcean();
    void DisableOcean();
    bool HasOcean();

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
    void CreateHouseOccluderTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<HouseOccluderTriangle>& triangles);
    void CreateHouseOccluderGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void UpdateHouseLightOccluderBvh();
	BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
    BvhRayResult ClosestHouseLightOccluderHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);

    const float GetWorldSpaceWidth();
    const float GetWorldSpaceDepth();

    // Map
    const std::string& GetCurrentMapName();

    // House
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

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t id, int32_t meshNodeLocalIndex);

    Shark* GetSharkByObjectId(uint64_t objectId);
    std::vector<Decal>& GetDecals();
    std::vector<HeightMapChunk>& GetHeightMapChunks();
    std::vector<MapInstance>& GetMapInstances();
    std::vector<SpawnPoint>& GetCampaignSpawnPoints();
    std::vector<SpawnPoint>& GetDeathmatchSpawnPoints();
    std::vector<Hell::Transform>& GetDoorAndWindowCubeTransforms();
    std::vector<Road>& GetRoads();
    std::vector<Shark>& GetSharks();
}
