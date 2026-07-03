#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"

namespace VulkanRenderer {

    void RenderLoadingScreen() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        frame.presentImage->Sync(frame.commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        RenderLoadingScreenPass(frame.commandBuffer, frame.presentImage->GetImageView(), frame.extent);

        UpdateBuffersUI();

        RenderUIPass(frame.commandBuffer);

        RenderPresentPass(frame.commandBuffer, frame.swapchainImageView);
        EndSwapchainFrame(frame);
    }
}
