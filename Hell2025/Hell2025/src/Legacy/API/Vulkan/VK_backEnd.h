#pragma once
#include "Hell/Common.h"

#include "Hell/ResourceManagement/Types/Texture.h"

struct UploadContext {
    VkFence uploadFence;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};

namespace VulkanBackEnd{
    bool Init();
    void Destroy();

    UploadContext& GetUploadContext();

    // Textures
    void UpdateTextureBaking();
    void AllocateTextureMemory(Texture& texture);
}
