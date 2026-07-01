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
#include "Unloved/Objects/House/House.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Renderables/MeshBufferOLD.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

#include <vector>

// get me out of here
#include "Unloved/Maps/Map.h"
#include "Util/Util.h"
//

struct MapCreateInfo {
    std::string mapName;
    uint32_t spawnOffsetChunkX;
    uint32_t spawnOffsetChunkZ;
};

namespace Unloved::LegacyWorld {

    void Init();
    void BeginFrame();
    void EndFrame();

    void NewRun();

    void SubmitRenderItems();

    void ResetWorld();
    void ClearAllObjects();

    DDGIVolume& GetTestDDGIVolume();

    void LoadMap(const std::string& mapName); // Calls the function below, but with a single map
    void LoadMaps(std::vector<MapCreateInfo> mapCreateInfoSet); // Calls the 3 functions below
    void LoadMapsHeightMapData(std::vector<MapCreateInfo> mapCreateInfoSet);
    void LoadMapObjects(const std::string& mapName, SpawnOffset spawnOffset);
    void LoadMapHouses(const std::string& mapName, SpawnOffset spawnOffset);

    void LoadSingleHouse(const std::string& houseName);
    void LoadHouse(const std::string& houseName, SpawnOffset spawnOffset);

    bool ChunkExists(int x, int z);
    const uint32_t GetChunkCountX();
    const uint32_t GetChunkCountZ();
    const uint32_t GetChunkCount();
    const HeightMapChunk* GetChunk(int x, int z);

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
    glm::vec3 GetGizmoOffest(uint64_t objectId);

    const float GetWorldSpaceWidth();
    const float GetWorldSpaceDepth();

    // Map
    const std::string& GetCurrentMapName();

    void RecreateAllHouseGeometry();
    void RecreateAllProceduralWallMesh();
    void RecreateAllProcedularWorldPlaneMesh();
    void RecreateAllWeatherBoards();
    void RecreateAllWallTrims();
    void RecreateAllHangingLightCords();
    void RemoveAllWeatherBoards();

    const glm::vec3& GetObjectPosition(uint64_t objectId);
    const glm::vec3& GetObjectRotation(uint64_t objectId);
    const std::string& GetObjectEditorName(uint64_t objectId);

    std::vector<HeightMapChunk>& GetHeightMapChunks();
    std::vector<Map>& GetMaps();
    std::vector<Road>& GetRoads();
}
