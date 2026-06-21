#include "AssetManager.h"
#include "BakeQueue.h"
#include "API/OpenGL/GL_backend.h"
#include "API/Vulkan/VK_backend.h"
#include "BackEnd/BackEnd.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Util/Util.h"
#include <future>

using namespace Hell;

namespace AssetManager {

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
        std::unordered_map<std::string, Texture>& textures = GetTextures();
        std::vector<std::future<void>> futures;

        for (auto& [name, texture] : textures) {
            if (texture.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                texture.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                futures.emplace_back(std::async(std::launch::async, LoadTexture, &texture));
            }
        }
        for (auto& future : futures) {
            future.get();
        }
        // Allocate gpu memory
        for (auto& [name, texture] : textures) {
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
        return ResourceManager::GetTextureByName(name);
    }

    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex) {
        return ResourceManager::GetTextureByBindlessIndex(bindlessIndex);
    }

    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning) {
        return ResourceManager::GetTextureBindlessIndexByName(name, ignoreWarning);
    }
}
