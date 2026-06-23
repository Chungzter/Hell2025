#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/TextureUploader/TextureUploader.h"

#include <cstring>
#include <utility>

namespace Hell::AssetLoader {

    bool g_loadingComplete = false;

    // TODO: Move to Image tools eventually
    ImageData LoadImageData(const std::string& path, ImageDataType type) {
        switch (type) {
            case ImageDataType::UNCOMPRESSED: return ImageTools::LoadUncompressedImage(path);
            case ImageDataType::COMPRESSED:   return ImageTools::LoadDDS(path);
            case ImageDataType::EXR:          return ImageTools::LoadEXRImage(path);
            default:
                Logging::Error() << "AssetLoader::LoadImageData() failed because image type was undefined for '" << path << "'\n";
                return {};
        }
    }

    void LoadFonts() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/fonts", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
            texture.SetImageData(Hell::AssetLoader::LoadImageData(fileInfo.path, ImageDataType::UNCOMPRESSED));

            if (!TextureUploader::ImmediateUpload(texture)) {
                Logging::Error() << "AssetLoader::LoadFonts() failed to upload '" << fileInfo.path << "'\n";
                continue;
            }

            texture.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }
    }

    void DiscoverAssets() {
        // IES Profiles
        for (FileInfo& fileInfo : File::IterateDirectory("res/ies_profiles", { "ies" })) {
            IESProfile& iesProfile = ResourceManager::CreateIESProfile(fileInfo.name);
            iesProfile.SetFileInfo(fileInfo);
        }
    }

    void Update() {
        g_loadingComplete = false;

        //if (!AllTexturesReadFromDisk()) {
        //    ReadNextTextureFromDisk();
        //    return;
        //}

        g_loadingComplete = true;
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

    bool LoadingComplete() {
        return g_loadingComplete;
    }
}
