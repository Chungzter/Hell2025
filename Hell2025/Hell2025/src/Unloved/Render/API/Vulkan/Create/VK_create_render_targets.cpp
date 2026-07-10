#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Unloved/Config/Config.h"

namespace VulkanRenderer {

    void CreateRenderTargets() {
        const Resolutions& resolutions = Config::GetResolutions();

        VkExtent2D gBufferExtent = { static_cast<uint32_t>(resolutions.gBuffer.x), static_cast<uint32_t>(resolutions.gBuffer.y) };
        VkExtent2D finalImageExtent = { static_cast<uint32_t>(resolutions.finalImage.x), static_cast<uint32_t>(resolutions.finalImage.y) };
        VkFormat finalImageFormat = VulkanSwapchainManager::GetSwapchainImageFormat();

        const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkImageUsageFlags hairUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // GBuffer
        VulkanResourceManager::CreateAllocatedImage("BaseColorMetallic", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("NormalXYRoughnessMisc", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_A2B10G10R10_UNORM_PACK32, usage);
        VulkanResourceManager::CreateAllocatedImage("VelocityXYOcclusionSubSurface", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("Visibility", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32_UINT, usage);
        VulkanResourceManager::CreateAllocatedImage("Lighting", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("Depth", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, depthUsage);

        // Hair
        VulkanResourceManager::CreateAllocatedImage("HairLighting", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_4_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, hairUsage);
        VulkanResourceManager::CreateAllocatedImage("HairDepth", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_4_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, depthUsage);

        // Final image
        VulkanResourceManager::CreateAllocatedImage("FinalImage", finalImageExtent.width, finalImageExtent.height, VK_SAMPLE_COUNT_1_BIT, finalImageFormat, usage);

        UpdateBindlessRenderTargetDescriptors();
    }

    void CreatePresentRenderTarget(VkExtent2D extent) {
        VkFormat format = VulkanSwapchainManager::GetSwapchainImageFormat();
        bool needsCreate = !VulkanResourceManager::AllocatedImageExists("Present");

        if (!needsCreate) {
            AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
            if (presentImage) {
                VkExtent2D presentExtent = presentImage->GetExtent2D();
                needsCreate = presentExtent.width != extent.width || presentExtent.height != extent.height || presentImage->GetFormat() != format || presentImage->GetSampleCount() != VK_SAMPLE_COUNT_1_BIT;
            }
            else {
                needsCreate = true;
            }
        }

        if (!needsCreate) return;

        if (VulkanResourceManager::AllocatedImageExists("Present")) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VulkanResourceManager::CreateAllocatedImage("Present", extent.width, extent.height, VK_SAMPLE_COUNT_1_BIT, format, usage);
        UpdateBindlessRenderTargetDescriptors();
    }
}
