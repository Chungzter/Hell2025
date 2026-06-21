#include "AssetManager.h"
#include "BakeQueue.h"
#include "API/OpenGL/GL_backend.h"
#include "API/Vulkan/VK_backend.h"
#include "BackEnd/BackEnd.h"
#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Util/Util.h"

#include <chrono>
#include <future>
#include <utility>
#include <vector>

using namespace Hell;

namespace AssetManager {

    namespace {
        std::vector<std::pair<Texture*, std::future<ImageData>>> g_textureLoadFutures;

        void FinishTextureLoad(Texture& texture) {
            texture.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);

            if (texture.GetTextureDataCount() == 0) {
                Logging::Error() << "AssetManager::FinishTextureLoad() received no image data for '" << texture.GetFilePath() << "'\n";
                return;
            }

            if (BackEnd::GetAPI() == API::OPENGL) {
                OpenGLBackEnd::AllocateTextureMemory(texture);
            }
            else if (BackEnd::GetAPI() == API::VULKAN) {
                // TODO: VulkanBackEnd::AllocateTextureMemory(texture);
            }

            BakeQueue::QueueTextureForBaking(&texture);
        }
    }

    void LoadPendingTexturesAsync() {
        for (auto& [name, texture] : GetTextures()) {
            if (texture.GetLoadingState() != LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                continue;
            }

            // Generated textures, such as IES data, already have their CPU image data.
            if (texture.GetTextureDataCount() > 0) {
                FinishTextureLoad(texture);
                continue;
            }

            texture.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);

            const std::string path = texture.GetFilePath();
            const ImageDataType type = texture.GetImageDataType();

            g_textureLoadFutures.emplace_back(
                &texture,
                std::async(std::launch::async, [path, type] {
                    return Hell::AssetLoader::LoadImageData(path, type);
                })
            );
        }
    }

    void UpdateTextureLoading() {
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
    }

    void LoadTexture(Texture* texture) {
        if (!texture) return;

        texture->SetImageData(
            Hell::AssetLoader::LoadImageData(texture->GetFilePath(), texture->GetImageDataType())
        );
        FinishTextureLoad(*texture);
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
