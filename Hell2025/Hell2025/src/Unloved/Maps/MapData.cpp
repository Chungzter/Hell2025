#include "MapData.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_util.h"

namespace Unloved {

    void MapData::CreateNew(const std::string& filename, int chunkCountX, int chunkCountZ, float initialHeight) {
        m_filename = filename;
        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            m_heightMapGLTexture.Create(GetTextureWidth(), GetTextureWidth(), GL_R16F, 1);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: HeightMap::CreateNew()";
        }

        ClearToHeight(initialHeight);
        Logging::Debug() << "Created map: '" << filename << "' with height map texture size " << GetTextureWidth() << "x" << std::to_string(GetTextureWidth());
    }

    void MapData::ClearToHeight(float height) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            int internalFormat = GL_R16F;   
            m_heightMapGLTexture.ClearR(height / HEIGHTMAP_SCALE_Y);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: MapData::ClearToHeight()\n";
        }
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

        //Logging::Debug() << "m_chunkCountX: " << m_chunkCountX;
        //Logging::Debug() << "m_chunkCountZ: " << m_chunkCountZ;

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            m_heightMapGLTexture.Create(GetTextureWidth(), GetTextureHeight(), GL_R16F, 1);
            m_heightMapGLTexture.UploadR16FData(m_heightMapData.data(), GetTextureWidth(), GetTextureHeight(), 0, 0, 0);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: MapData::SetHeightMapData()";
        }
    }

    void MapData::SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection) {
        m_createInfoCollection = createInfoCollection;
    }

    void MapData::SetAdditionalMapData(const AdditionalMapData& additionalMapData) {
        m_additionalMapData = additionalMapData;
    }

    const glm::ivec2 MapData::GetHeightMapTextureSize() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return glm::ivec2(m_heightMapGLTexture.GetWidth(), m_heightMapGLTexture.GetHeight());
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: MapData::GetHeightMapTextureSize()\n";
            return glm::ivec2(0, 0);
        }
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
