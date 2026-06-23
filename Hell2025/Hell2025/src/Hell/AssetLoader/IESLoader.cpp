#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"

#include <tinyies/tiny_ies.hpp>

#include <utility>

namespace Hell::AssetLoader {

    bool LoadIES(const std::string& path, IESProfile& outProfile) {
        tiny_ies<float>::light ies;
        std::string error;
        std::string warning;

        if (!tiny_ies<float>::load_ies(path, error, warning, ies)) {
            Logging::Error() << "AssetLoader::LoadIES() failed to load '" << path << "': " << error << "\n";
            return false;
        }

        if (!warning.empty()) {
            Logging::Warning() << "AssetLoader::LoadIES() warning for '" << path << "': " << warning << "\n";
        }

        IESProfile loadedProfile(outProfile.m_name);
        loadedProfile.m_fileInfo = outProfile.m_fileInfo;
        loadedProfile.m_textureIndex = outProfile.m_textureIndex;
        loadedProfile.m_verticalAngles = std::move(ies.vertical_angles);
        loadedProfile.m_horizontalAngles = std::move(ies.horizontal_angles);
        loadedProfile.m_candelaValues = std::move(ies.candela);
        loadedProfile.m_horizontalAngleCount = ies.number_horizontal_angles;
        loadedProfile.m_verticalAngleCount = ies.number_vertical_angles;
        loadedProfile.m_maxIntensity = ies.max_candela;
        loadedProfile.RecalculateDerivedValues();

        outProfile = std::move(loadedProfile);
        return true;
    }

    void LoadIESFiles() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/ies_profiles", { "ies" })) {
            IESProfile& iesProfile = ResourceManager::GetIESProfile(fileInfo.name);

            if (!LoadIES(fileInfo.path, iesProfile)) {
                continue;
            }

            const uint32_t width = static_cast<uint32_t>(iesProfile.GetVerticalAngleCount());
            const uint32_t height = static_cast<uint32_t>(iesProfile.GetHorizontalAngleCount());
            const std::vector<float>& candelaValues = iesProfile.GetCandelaValues();
            const size_t expectedValueCount = static_cast<size_t>(width) * static_cast<size_t>(height);

            if (width == 0 || height == 0 || candelaValues.size() != expectedValueCount) {
                Logging::Error() << "AssetLoader::LoadIESFiles(..) failed to create texture data for '" << fileInfo.name << "' because the candela grid dimensions are invalid\n";
                continue;
            }

            ImageData imageData;
            imageData.format = ImageFormat::R32_SFLOAT;

            TextureMip& mip = imageData.mips.emplace_back();
            mip.width = width;
            mip.height = height;
            mip.data.resize(candelaValues.size() * sizeof(float));
            std::memcpy(mip.data.data(), candelaValues.data(), mip.data.size());

            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageData(std::move(imageData));
            texture.SetTextureWrapModeS(TextureWrapMode::REPEAT);
            texture.SetTextureWrapModeT(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            iesProfile.SetTextureIndex(texture.GetBindlessIndex());
        }
    }
}
