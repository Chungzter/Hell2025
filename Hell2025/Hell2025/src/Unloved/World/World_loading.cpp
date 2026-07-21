#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Hell/Common/Constants.h"
#include "Hell/Logging.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Objects/House/HouseData.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Map/MapManager.h"

namespace Unloved::World {
    void LoadMap(const std::string& mapName) {
        MapCreateInfo createInfo;
        createInfo.mapName = mapName;
        LoadMaps({ createInfo });
    }

    void LoadMaps(const std::vector<MapCreateInfo>& mapCreateInfoSet) {
        LegacyWorld::LoadMapsHeightMapData(mapCreateInfoSet);

        for (const MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            SpawnOffset spawnOffset;
            spawnOffset.translation.x = mapCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
            spawnOffset.translation.z = mapCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;

            MapData* mapData = MapManager::GetMapDataByName(mapCreateInfo.mapName);
            if (!mapData) {
                Logging::Error() << "World::LoadMaps() failed coz '" << mapCreateInfo.mapName << "' was not found";
                continue;
            }

            LoadMap(*mapData, spawnOffset);
        }

        KangarooCreateInfo kangarooCreateInfo;
        kangarooCreateInfo.position = glm::vec3(48, 32.6, 39);
        kangarooCreateInfo.rotation = glm::vec3(0, HELL_PI * -0.5f, 0);
        AddKangaroo(kangarooCreateInfo);
    }

    void LoadMap(const MapData& mapData, SpawnOffset spawnOffset) {
        LoadMapObjects(mapData, spawnOffset);
        LoadMapHouses(mapData, spawnOffset);
    }

    void LoadMapObjects(const MapData& mapData, SpawnOffset spawnOffset) {
        AddCreateInfoCollection(mapData.GetCreateInfoCollection(), spawnOffset);
    }

    void LoadMapHouses(const MapData& mapData, SpawnOffset spawnOffset) {
        for (const HouseLocation& houseLocation : mapData.GetAdditionalMapData().houseLocations) {
            SpawnOffset houseSpawnOffset = spawnOffset;
            houseSpawnOffset.translation += houseLocation.position;
            houseSpawnOffset.yRotation += houseLocation.rotation;

            LoadHouse("TestHouse", houseSpawnOffset);
        }
    }

    void LoadSingleHouse(const std::string& houseName) {
        ResetWorld();
        LoadHouse(houseName, SpawnOffset());
    }

    void LoadHouse(const std::string& houseName, SpawnOffset spawnOffset) {
        HouseData* houseData = HouseManager::GetHouseDataByName(houseName);
        if (!houseData) {
            Logging::Error() << "World::LoadHouse() failed because " << houseName << " was not found";
            return;
        }

        LoadHouse(*houseData, spawnOffset);
    }

    void LoadHouse(const HouseData& houseData, SpawnOffset spawnOffset) {
        AddCreateInfoCollection(houseData.GetCreateInfoCollection(), spawnOffset);

        Logging::Debug() << "World::LoadHouse(): " << houseData.GetFilename() << " at " << spawnOffset.translation;
    }
}
