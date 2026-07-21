#include "MapData.h"

#include "Hell/Logging.h"

namespace Unloved {

    void MapData::CreateNew(const std::string& filename, int chunkCountX, int chunkCountZ, float initialHeight) {
        m_filename = filename;
        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;

        ClearToHeight(initialHeight);
        Logging::Debug() << "Created map: '" << filename << "' with height map size " << GetTextureWidth() << "x" << GetTextureHeight();
    }

    void MapData::ClearToHeight(float height) {
        m_heightMapData.assign(static_cast<size_t>(GetTextureWidth()) * static_cast<size_t>(GetTextureHeight()), height / HEIGHTMAP_SCALE_Y);
    }

    void MapData::SetFilename(const std::string& filename) {
        m_filename = filename;
    }

    void MapData::SetChunkCountX(int32_t count) {
        m_chunkCountX = count;
    }

    void MapData::SetChunkCountZ(int32_t count) {
        m_chunkCountZ = count;
    }

    void MapData::SetHeightMapData(int32_t chunkCountX, int32_t chunkCountZ, const std::vector<float>& data) {
        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;
        m_heightMapData = data;
    }

    void MapData::SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection) {
        m_createInfoCollection = createInfoCollection;
    }

    void MapData::SetAdditionalMapData(const AdditionalMapData& additionalMapData) {
        m_additionalMapData = additionalMapData;
    }

    const glm::ivec2 MapData::GetHeightMapTextureSize() {
        return glm::ivec2(GetTextureWidth(), GetTextureHeight());
    }


    void MapData::AddPlayerCampaignSpawn(glm::vec3 position) {
        SpawnPointCreateInfo& spawnPoint = m_createInfoCollection.spawnPointsCampaign.emplace_back();
        spawnPoint.position = position;
        spawnPoint.camEuler = glm::vec3(0.0f);
    }

    void MapData::AddPlayerDeathmatchSpawn(glm::vec3 position) {
        SpawnPointCreateInfo& spawnPoint = m_createInfoCollection.spawnPointsDeathMatch.emplace_back();
        spawnPoint.position = position;
        spawnPoint.camEuler = glm::vec3(0.0f);
    }

}
