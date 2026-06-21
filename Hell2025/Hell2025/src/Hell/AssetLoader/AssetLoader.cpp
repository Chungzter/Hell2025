#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <cstring>
#include <utility>

namespace Hell::AssetLoader {

    void DiscoverAssets() {
        // IES Profiles
        for (FileInfo& fileInfo : File::IterateDirectory("res/ies_profiles", { "ies" })) {
            IESProfile& iesProfile = ResourceManager::CreateIESProfile(fileInfo.name);
            iesProfile.SetFileInfo(fileInfo);
        }
    }

    void LoadIESFiles() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/ies_profiles", { "ies" })) {
            IESProfile& iesProfile = ResourceManager::GetIESProfile(fileInfo.name);

            if (!File::LoadIESProfile(fileInfo.path, iesProfile)) {
                continue;
            }

            const uint32_t width = static_cast<uint32_t>(iesProfile.GetVerticalAngleCount());
            const uint32_t height = static_cast<uint32_t>(iesProfile.GetHorizontalAngleCount());
            const std::vector<float>& candelaValues = iesProfile.GetCandelaValues();
            const size_t expectedValueCount = static_cast<size_t>(width) * static_cast<size_t>(height);

            if (width == 0 || height == 0 || candelaValues.size() != expectedValueCount) {
                Logging::Error() << "AssetLoader::LoadIESFiles() failed to create texture data for '" << fileInfo.name << "' because the candela grid dimensions are invalid\n";
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
