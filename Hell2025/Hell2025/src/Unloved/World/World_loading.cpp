#include "World.h"

#include "Hell/Logging.h"

#include "Unloved/Maps/MapData.h"
#include "Unloved/Objects/House/HouseData.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Ocean/Ocean.h"

namespace Unloved::World {
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

            HouseData* houseData = HouseManager::GetHouseDataByName("TestHouse");
            if (!houseData) {
                Logging::Error() << "World::LoadMap() failed to load house at " << houseLocation.position;
                continue;
            }

            LoadHouse(*houseData, houseSpawnOffset);
        }
    }

    void LoadHouse(const HouseData& houseData, SpawnOffset spawnOffset) {
        // TODO: Probably handle me better!
        Ocean::CreatePhysicsPlane();

        AddCreateInfoCollection(houseData.GetCreateInfoCollection(), spawnOffset);

        Logging::Debug() << "World::LoadHouse(): " << houseData.GetFilename() << " at " << spawnOffset.translation;
    }
}
