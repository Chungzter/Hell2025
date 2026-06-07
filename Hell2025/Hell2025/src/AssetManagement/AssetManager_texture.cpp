#include "AssetManager.h"
#include "BakeQueue.h"
#include "API/OpenGL/GL_backend.h"
#include "API/Vulkan/VK_backend.h"
#include "BackEnd/BackEnd.h"
#include "Hell/Logging.h"
#include "Tools/ImageTools.h"
#include "Util/Util.h"
#include <future>

namespace AssetManager {

    std::unordered_map<std::string, Texture*> g_cachedTexturePointers;
    std::unordered_map<std::string, int> g_cachedTextureIndices;

    void ClearCachedTextureMaps() {
        g_cachedTexturePointers.clear();
        g_cachedTextureIndices.clear();
    }

    void CompressMissingDDSTexutres() {
        for (FileInfo& fileInfo : Util::IterateDirectory("res/textures/compress_me", { "png", "jpg", "tga" })) {
            std::string inputPath = fileInfo.path;
            std::string outputPath = "res/textures/compressed/" + fileInfo.name + ".dds";
            if (!Util::FileExists(outputPath)) {
                ImageTools::CreateAndExportDDS(inputPath, outputPath, true);
                std::cout << "Exported " << outputPath << "\n";
            }
        }
    }

    void LoadPendingTexturesAsync() {
        std::vector<Texture>& textures = GetTextures();
        std::vector<std::future<void>> futures;

        for (Texture& texture : textures) {
            if (texture.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                texture.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                futures.emplace_back(std::async(std::launch::async, LoadTexture, &texture));
            }
        }
        for (auto& future : futures) {
            future.get();
        }
        // Allocate gpu memory
        for (Texture& texture : textures) {
            if (BackEnd::GetAPI() == API::OPENGL) {
                OpenGLBackEnd::AllocateTextureMemory(texture);
            }
            else if (BackEnd::GetAPI() == API::VULKAN) {
                // TODO : VulkanBackEnd::AllocateTextureMemory(texture);
            }
        }
    }

    void LoadTexture(Texture* texture) {
        if (texture) {
            texture->Load();
            BakeQueue::QueueTextureForBaking(texture);
        }
    }

    Texture* GetTextureByName(const std::string& name) {
        auto it = g_cachedTexturePointers.find(name);
        if (it != g_cachedTexturePointers.end()) {
            return it->second;
        }

        for (Texture& texture : GetTextures()) {
            if (texture.GetFileInfo().name == name) {
                g_cachedTexturePointers[name] = &texture;
                return &texture;
            }
        }

        Logging::Fatal() << "AssetManager::GetTextureByName(const std::string& name) failed because '" << name << "' does not exist\n";
        return nullptr;
    }

    Texture* GetTextureByIndex(int index) {
        if (index < 0 || index >= GetTextureCount()) {
            std::cout << "AssetManager::GetTextureByIndex() failed because index '" << index << "' was out of range of size " << GetTextureCount() << "\n";
            return nullptr;
        }

        return &GetTextures()[index];
    }

    int GetTextureIndexByName(const std::string& name, bool ignoreWarning) {
        auto it = g_cachedTextureIndices.find(name);
        if (it != g_cachedTextureIndices.end()) {
            return it->second;
        }

        std::vector<Texture>& textures = GetTextures();

        for (int i = 0; i < textures.size(); i++) {
            if (textures[i].GetFileInfo().name == name) {
                g_cachedTextureIndices[name] = i;
                return i;
            }
        }
        
        if (!ignoreWarning) {
            Logging::Fatal() << "AssetManager::GetTextureIndexByName(const std::string& name) failed because '" << name << "' does not exist\n";
        }

        return -1;
    }

    size_t GetTextureCount() {
        return GetTextures().size();
    }
}