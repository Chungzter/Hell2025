#include "AssetManager.h"
#include "BakeQueue.h"
#include "API/OpenGL/GL_backend.h"
#include "API/Vulkan/VK_backend.h"
#include "BackEnd/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Util/Util.h"
#include <future>

using namespace Hell;

namespace AssetManager {

    std::unordered_map<std::string, uint64_t> g_cachedTextureIds;
    std::unordered_map<uint64_t, size_t> g_cachedTextureStorageIndices;

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
        const uint64_t id = GetTextureIdByName(name);
        return id == 0 ? nullptr : GetTextureById(id);
    }

    Texture* GetTextureById(uint64_t id) {
        auto cachedIt = g_cachedTextureStorageIndices.find(id);
        if (cachedIt != g_cachedTextureStorageIndices.end()) {
            const size_t storageIndex = cachedIt->second;
            std::vector<Texture>& textures = GetTextures();

            if (storageIndex < textures.size() && textures[storageIndex].GetId() == id) {
                return &textures[storageIndex];
            }

            g_cachedTextureStorageIndices.erase(cachedIt);
        }

        std::vector<Texture>& textures = GetTextures();

        for (size_t i = 0; i < textures.size(); i++) {
            if (textures[i].GetId() == id) {
                g_cachedTextureStorageIndices[id] = i;
                return &textures[i];
            }
        }

        Logging::Error() << "AssetManager::GetTextureById() failed because texture id '" << id << "' does not exist\n";
        return nullptr;
    }

    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex) {
        for (Texture& texture : GetTextures()) {
            if (texture.GetBindlessIndex() == bindlessIndex) {
                return &texture;
            }
        }

        Logging::Error() << "AssetManager::GetTextureByBindlessIndex() failed because bindless index '" << bindlessIndex << "' does not exist\n";
        return nullptr;
    }

    uint64_t GetTextureIdByName(const std::string& name, bool ignoreWarning) {
        auto cachedIt = g_cachedTextureIds.find(name);
        if (cachedIt != g_cachedTextureIds.end()) {
            return cachedIt->second;
        }

        for (Texture& texture : GetTextures()) {
            if (texture.GetFileInfo().name == name) {
                const uint64_t id = texture.GetId();
                g_cachedTextureIds[name] = id;
                return id;
            }
        }

        if (!ignoreWarning) {
            Logging::Error() << "AssetManager::GetTextureIdByName() failed because texture '" << name << "' does not exist\n";
        }

        return 0;
    }

    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning) {
        const uint64_t id = GetTextureIdByName(name, true);

        if (id != 0) {
            if (Texture* texture = GetTextureById(id)) {
                return texture->GetBindlessIndex();
            }
        }

        if (!ignoreWarning) {
            Logging::Fatal() << "AssetManager::GetTextureBindlessIndexByName() failed because texture '" << name << "' does not exist\n";
        }

        return -1;
    }

    size_t GetTextureCount() {
        return GetTextures().size();
    }
}
