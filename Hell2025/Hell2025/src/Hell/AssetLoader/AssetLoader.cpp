#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/TextureUploader.h"

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

    void LoadRequired() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/fonts", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.SetImageData(Hell::ImageTools::LoadImageData(fileInfo.path, ImageDataType::UNCOMPRESSED));

            if (!TextureUploader::ImmediateUpload(texture)) {
                Logging::Error() << "AssetLoader::LoadRequired(..) failed to upload '" << fileInfo.path << "'\n";
                continue;
            }

            texture.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }

        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/required", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.SetImageData(Hell::ImageTools::LoadImageData(fileInfo.path, ImageDataType::UNCOMPRESSED));
            texture.RequestMipmaps();

            if (!TextureUploader::ImmediateUpload(texture)) {
                Logging::Error() << "AssetLoader::LoadRequired(..) failed to upload '" << fileInfo.path << "'\n";
                continue;
            }

            texture.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }
    }

    void DiscoverAssets() {
        // Animations
        for (FileInfo& fileInfo : File::IterateDirectory("res/animations")) {
            Animation& animation = ResourceManager::CreateAnimation(fileInfo.name);
            animation.SetFileInfo(fileInfo);
        }

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
            g_textureLoadFutures.emplace_back(&texture, std::async(std::launch::async, [path, type] { return ImageTools::LoadImageData(path, type); }));
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

    bool LoadingComplete() {
        return g_loadingComplete;
    }
}
