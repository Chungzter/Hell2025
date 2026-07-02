#include "MapManager.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include "Legacy/File/JSON.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Maps/MapFile.h"
#include "Unloved/World/World.h"

#include <cstring>
#include <fstream>

namespace Unloved::MapManager {
    std::vector<MapData> g_mapData;

    void Init() {
        //NewMap("Shit", 8, 16, 30.0f);
        g_mapData.clear();
        LoadMapData("Shit");
    }

    void NewMap(const std::string& name, int chunkWidth, int chunkDepth, float initialHeight) {
        MapData& mapData = g_mapData.emplace_back();
        mapData.CreateNew(name, chunkWidth, chunkDepth, initialHeight);
    }

    void SaveMap(const std::string& mapName) {
        MapData* mapData = GetMapDataByName(mapName);
        if (!mapData) {
            Logging::Error() << "SaveMap(): failed because '" << mapName << "' was not found.";
            return;
        }

        // Get heightmap data
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            Renderer::ReadBackHeightMapData(mapData);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: MapManager::SaveHeightMap()";
            return;
        }

        int32_t textureWidth = mapData->GetTextureWidth();
        int32_t textureHeight = mapData->GetTextureHeight();
        int32_t floatCount = textureWidth * textureHeight;
        int32_t dataSize = mapData->GetHeightMapData().size();

        // Validate height map data size
        if (dataSize != floatCount) {
            Logging::Error() << "File::SaveHeightMap() failed because map.m_heightMapData.size() is " << dataSize << " but width(" << textureWidth << ") * height(" << textureHeight << ") equals " << floatCount;
            return;
        }

        // Construct the JSON string
        CreateInfoCollection createInfoCollection = World::GetCreateInfoCollection();
        mapData->SetCreateInfoCollection(createInfoCollection);
        
        std::string createInfoJson = JSON::CreateInfoCollectionToJSON(createInfoCollection);
        std::string additionalJson = JSON::AdditionalMapDataToJSON(mapData->GetAdditionalMapData());

        // Create the file
        std::string outputPath = "res/maps/" + mapName + ".map";
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            Logging::Error() << "Failed to open file for writing: " << outputPath << "\n";
            return;
        }

        // Write the header
        MapHeader header{};
        header.version = 1;
        header.chunkCountX = mapData->GetChunkCountX();
        header.chunkCountZ = mapData->GetChunkCountZ();
        header.createInfoJsonLength = createInfoJson.size();
        header.additionalJsonLength = additionalJson.size();
        MapFile::CopySignature(header.signature, HELL_MAP_SIGNATURE);
        file.write(reinterpret_cast<const char*>(&header), sizeof(MapHeader));

        // Write the height map pixel data
        file.write(reinterpret_cast<const char*>(mapData->GetHeightMapData().data()), floatCount * sizeof(float));

        // Write JSON blobs immediately after
        file.write(createInfoJson.data(), static_cast<std::streamsize>(createInfoJson.size()));
        file.write(additionalJson.data(), static_cast<std::streamsize>(additionalJson.size()));

        // Close file
        file.close();

        Logging::Debug() << "Saved " << outputPath;

        //Logging::Debug()
        //    << "Saved map '" << mapName << "'\n"
        //    << createInfoJson << "'\n"
        //    << additionalJson;
    }

    void LoadMapData(const std::string& mapName) {
        const std::string path = "res/maps/" + mapName + ".map";
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "LoadMapData(): failed to open '" << path << "'";
            return;
        }

        MapHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file) {
            Logging::Error() << "LoadMapData(): failed reading header";
            return;
        }

        // Validate header signature
        if (std::memcmp(header.signature, HELL_MAP_SIGNATURE, sizeof(HELL_MAP_SIGNATURE)) != 0) {
            Logging::Error() << "LoadMapData(): bad file signature";
            return;
        }

        uint32_t textureWidth = header.chunkCountX * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
        uint32_t textureHeight = header.chunkCountZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
        uint32_t floatCount = textureWidth * textureHeight;
        std::vector<float> heightMapData(floatCount);

        // Read height map data
        file.read(reinterpret_cast<char*>(heightMapData.data()), static_cast<std::streamsize>(floatCount * sizeof(float)));
        if (!file) {
            Logging::Error() << "LoadMapData(): failed reading height data";
            return;
        }

        MapData& mapData = g_mapData.emplace_back();
        mapData.SetFilename(mapName);
        mapData.SetHeightMapData(header.chunkCountX, header.chunkCountZ, heightMapData);

        std::string createInfoJson;
        std::string additionalJson;

        createInfoJson.resize(header.createInfoJsonLength);
        additionalJson.resize(header.additionalJsonLength);

        if (header.createInfoJsonLength > 0) {
            file.read(createInfoJson.data(), static_cast<std::streamsize>(header.createInfoJsonLength));
            if (!file) {
                Logging::Error() << "LoadMapData(): failed reading create info json";
                return;
            }
        }

        if (header.additionalJsonLength > 0) {
            file.read(additionalJson.data(), static_cast<std::streamsize>(header.additionalJsonLength));
            if (!file) {
                Logging::Error() << "LoadMapData(): failed reading additional json";
                return;
            }
        }

        // Load Create Info Collection from JSON string
        CreateInfoCollection createInfoCollection = JSON::CreateInfoCollectionFromJSONString(createInfoJson);
        AdditionalMapData additionalMapData = JSON::AdditionalMapDataFromJSON(additionalJson);

        mapData.SetCreateInfoCollection(createInfoCollection);
        mapData.SetAdditionalMapData(additionalMapData);

        Logging::Debug()
            << "Loaded map: " << mapName << ".map\n"
            //<< "- signature:     " << header.signature << "\n"
            //<< "- version:       " << header.version << "\n"
            //<< "- chunk count x: " << header.chunkCountX << "\n"
            //<< "- chunk count z: " << header.chunkCountZ << "\n"
            //<< createInfoJson << "\n"
            //<< additionalJson;
            << "";

        return;
    }

    void UpdateCreateInfoCollectionFromWorld(const std::string& mapName) {
        MapData* mapData = GetMapDataByName(mapName);
        if (!mapData) {
            Logging::Error() << "MapManager::UpdateCreateInfoCollectionFromWorld(): failed because '" << mapName << "' was not found.";
            return;
        }

        CreateInfoCollection createInfoCollection = World::GetCreateInfoCollection();
        mapData->SetCreateInfoCollection(createInfoCollection);
    }

    MapData* GetTestMapData() {
        return GetMapDataByName("Shit");
    }

    MapData* GetMapDataByIndex(int32_t index) {
        if (index < 0 || index >= static_cast<int32_t>(g_mapData.size())) {
            Logging::Error() << "MapManager::GetMapDataByIndex() failed coz '" << index << "' is out of range of size " << g_mapData.size();
            return nullptr;
        }
        return &g_mapData[index];
    }

    MapData* GetMapDataByName(const std::string& name) {
        for (MapData& mapData : g_mapData) {
            if (mapData.GetFilename() == name) {
                return &mapData;
            }
        }
        Logging::Error() << "MapManager::GetMapDataByName() failed coz '" << name << "' was not found";
        return nullptr;
    }

    int32_t GetMapDataIndexByName(const std::string& name) {
        for (int i = 0; i < g_mapData.size(); i++) {
            if (g_mapData[i].GetFilename() == name) {
                return (int32_t)i;
            }
        }
        Logging::Error() << "MapManager::GetMapDataIndexByName() failed coz '" << name << "' was not found";
        return -1;
    }
}
