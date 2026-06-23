#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/TextureUploader/TextureUploader.h"

#include <chrono>
#include <cstring>
#include <future>
#include <utility>
#include <vector>

namespace Hell::AssetLoader {

    namespace {
        bool g_loadingComplete = false;
        std::vector<std::pair<Texture*, std::future<ImageData>>> g_textureLoadFutures;

        void FinishTextureLoad(Texture& texture) {
            if (texture.GetTextureDataCount() == 0) {
                Logging::Error() << "AssetLoader::FinishTextureLoad(..) received no image data for '" << texture.GetFilePath() << "'\n";
                return;
            }

            if (SpriteSheetTexture* spriteSheetTexture = ResourceManager::GetSpriteSheetTexturePtr(texture.GetFileName())) {
                spriteSheetTexture->Init(texture);
            }

            TextureUploader::QueueUpload(texture);
            texture.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }
    }

    // TODO: Move to Image tools eventually
    ImageData LoadImageData(const std::string& path, ImageDataType type) {
        switch (type) {
            case ImageDataType::UNCOMPRESSED: return ImageTools::LoadUncompressedImage(path);
            case ImageDataType::COMPRESSED:   return ImageTools::LoadDDS(path);
            case ImageDataType::EXR:          return ImageTools::LoadEXRImage(path);
            default:
                Logging::Error() << "AssetLoader::LoadImageData(..) failed because image type was undefined for '" << path << "'\n";
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
                Logging::Error() << "AssetLoader::LoadFonts(..) failed to upload '" << fileInfo.path << "'\n";
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

        // Textures (Uncompressed)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/uncompressed", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();
        }

        // Textures (Decals)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/decals", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_BORDER);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();
        }

        // Textures (Compressed)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/compressed", { "dds" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::COMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();
        }

        // Textures (UI)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/ui", { "png", "jpg", })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);
        }

        // Textures (EXR)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/exr", { "exr" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::EXR);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::NEAREST);
        }

        // Textures (Spritesheets)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/spritesheets", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            SpriteSheetTexture& spriteSheetTexture = ResourceManager::CreateSpriteSheetTexture(fileInfo.name);
            spriteSheetTexture.SetFileInfo(fileInfo);
        }
    }

    void Update() {
        g_loadingComplete = false;

        for (auto& [name, texture] : ResourceManager::GetTextures()) {
            if (texture.GetLoadingState() != LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                continue;
            }

            if (texture.GetTextureDataCount() > 0) {
                FinishTextureLoad(texture);
                continue;
            }

            texture.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);

            const std::string path = texture.GetFilePath();
            const ImageDataType type = texture.GetImageDataType();
            g_textureLoadFutures.emplace_back(&texture, std::async(std::launch::async, [path, type] { return LoadImageData(path, type); }));
        }

        for (size_t i = 0; i < g_textureLoadFutures.size();) {
            Texture* texture = g_textureLoadFutures[i].first;
            std::future<ImageData>& future = g_textureLoadFutures[i].second;

            if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                ++i;
                continue;
            }

            texture->SetImageData(future.get());
            FinishTextureLoad(*texture);
            g_textureLoadFutures.erase(g_textureLoadFutures.begin() + i);
        }

        g_loadingComplete = g_textureLoadFutures.empty();

        for (auto& [name, texture] : ResourceManager::GetTextures()) {
            if (texture.GetLoadingState() != LoadingState::Value::LOADING_COMPLETE || !texture.BakeComplete()) {
                g_loadingComplete = false;
                break;
            }
        }
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

    bool LoadingComplete() {
        return g_loadingComplete;
    }
}
