#pragma once
#include "Unloved/Render/API/Vulkan/VK_frame_data.h"
#include "Hell/Render/API/Vulkan/vk_common.h"

namespace VulkanRenderer {
    void Init();
    void InitMain();
    void CleanUp();
    void RenderLoadingScreen();
    void RenderGame();

    VulkanFrameData& GetCurrentFrameData();
    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex);
    uint32_t GetCurrentFrameIndex();

    void UpdateBindlessTextureDescriptors();
    void UpdateUIBuffers();
    void RenderUIPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);
}
