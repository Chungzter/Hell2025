#include "VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Render/DrawCommandTypes.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_deletion_queue.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_sync_manager.h"
#include "Hell/Render/API/Vulkan/vk_tools.h"
#include "Hell/Render/API/Vulkan/vk_types.h"
#include "Hell/ResourceManagement/Types/Material.h"
#include "Hell/UI/UITypes.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/RendererTypes.h"

#include <array>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <vector>

namespace VulkanRenderer {
    uint32_t g_frameIndex = 0;
    std::array<VulkanFrameData, FRAME_OVERLAP> g_frameData;
    std::vector<VkImageLayout> g_swapchainImageLayouts;
    bool g_staticSamplersUploaded = false;

    void CreateSamplers();
    void CreateStaticDescriptorSet();
    void CreateFrameData();

    void Init() {
        CreateShaders();
        CreateSamplers();
        CreateStaticDescriptorSet();
        CreateFrameData();
        CreateRenderTargets();
        CreatePresentRenderTarget(VulkanSwapchainManager::GetSwapchainExtent());
        CreateRenderStates();
        CreatePipelines();
        UpdateBindlessTextureDescriptors();
    }

    void InitMain() {
        UpdateBindlessTextureDescriptors();
    }

    void CleanUp() {
        if (VulkanDeviceManager::GetDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        ProfilerVulkanReset();
        VulkanDeletionQueue::FlushAll();
        VulkanResourceManager::Cleanup();
    }

    void HotloadShaders() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        if (device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(device);

        std::string failedShaders = "FAILED TO HOTLOAD";
        if (!VulkanResourceManager::HotloadShaders(failedShaders)) {
            Debug::BlitQuickDebugMessage(failedShaders);
            return;
        }

        VulkanResourceManager::CleanUpPipelines();
        CreatePipelines();

        std::cout << "Hotloaded shaders\n";
        Debug::BlitQuickDebugMessage("HOTLOADED SHADERS");
    }

    void CreateSamplers() {
        const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();
        const float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        VulkanResourceManager::CreateSampler("Linear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, maxAnisotropy);
        VulkanResourceManager::CreateSampler("Nearest", VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    }

    void CreateStaticDescriptorSet() {
        g_staticSamplersUploaded = false;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            { DESC_IDX_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER, 16, VK_SHADER_STAGE_ALL },
            { DESC_IDX_TEXTURES, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000, VK_SHADER_STAGE_ALL },
            { DESC_IDX_UBOS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128, VK_SHADER_STAGE_ALL },
            { DESC_IDX_SSBOS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA32F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA16F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA8, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_UINT_TEXTURES, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128, VK_SHADER_STAGE_ALL },
            { DESC_IDX_TEXTURE_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER, 10000, VK_SHADER_STAGE_ALL },
            { DESC_IDX_ACCELERATION_STRUCTURES, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_ALL }
        };

        std::vector<VkDescriptorBindingFlags> flags(bindings.size(), 0);
        flags[DESC_IDX_TEXTURES] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_STORAGE_IMAGES_RGBA32F] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_STORAGE_IMAGES_RGBA16F] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_STORAGE_IMAGES_RGBA8] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_UINT_TEXTURES] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_TEXTURE_SAMPLERS] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        flags[DESC_IDX_ACCELERATION_STRUCTURES] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        flagsInfo.bindingCount = static_cast<uint32_t>(flags.size());
        flagsInfo.pBindingFlags = flags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.pNext = &flagsInfo;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VulkanResourceManager::CreateDescriptorSet("StaticDescriptorSet", layoutInfo, DescriptorSetLifetime::STATIC);
    }

    void CreateFrameData() {
        VkBufferUsageFlags usageStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VkBufferUsageFlags usageIndirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VkBufferUsageFlags usageSkinnedVertices = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBufferUsageFlags usageRayQueryInstances = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VkBufferUsageFlags usageRayQueryScratch = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        for (VulkanFrameData& frameData : g_frameData) {
            frameData.buffers.instanceData = VulkanResourceManager::CreateBuffer(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.viewportData = VulkanResourceManager::CreateBuffer(sizeof(ViewportData) * MAX_VIEWPORT_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rendererData = VulkanResourceManager::CreateBuffer(sizeof(RendererData), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.gpuLights = VulkanResourceManager::CreateBuffer(sizeof(GPULight) * 8, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.materials = VulkanResourceManager::CreateBuffer(sizeof(Material), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.drawCommands = VulkanResourceManager::CreateBuffer(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT, usageIndirect, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinningTransforms = VulkanResourceManager::CreateBuffer(sizeof(glm::mat4) * MAX_ANIMATED_TRANSFORMS, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinnedVertices = VulkanResourceManager::CreateBuffer(sizeof(Vertex), usageSkinnedVertices, VMA_MEMORY_USAGE_GPU_ONLY);
            frameData.buffers.rayQueryInstances = VulkanResourceManager::CreateBuffer(1, usageRayQueryInstances, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryInstanceData = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryGeometryData = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryScratch = VulkanResourceManager::CreateBuffer(1, usageRayQueryScratch, VMA_MEMORY_USAGE_AUTO);
            frameData.buffers.uiRenderItems = VulkanResourceManager::CreateBuffer(sizeof(RenderItemUI) * VULKAN_MAX_UI_RENDER_ITEMS, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.accelerationStructures.rayQueryTLAS = VulkanResourceManager::CreateAccelerationStructure();
        }
    }

    bool UpdateBuffer(VulkanBuffer* buffer, const void* data, VkDeviceSize size) {
        if (!buffer) return false;

        if (size > buffer->GetSize()) {
            Logging::Error() << "VulkanRenderer::UpdateBuffer() data exceeded the current Vulkan buffer capacity\n";
            return false;
        }

        buffer->UpdateData(data, size);
        return true;
    }

    bool EnsureBufferSize(uint64_t id, VkDeviceSize size) {
        if (size == 0) return true;

        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(id);
        if (!buffer) return false;

        return buffer->EnsureSize(size);
    }

    VulkanFrameData& GetCurrentFrameData() {
        return g_frameData[g_frameIndex % FRAME_OVERLAP];
    }

    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex) {
        return g_frameData[frameIndex % FRAME_OVERLAP];
    }

    uint32_t GetCurrentFrameIndex() {
        return g_frameIndex % FRAME_OVERLAP;
    }

    const std::string& GetZoneNames() {
        return ProfilerVulkanZoneNames();
    }

    const std::string& GetZoneGPUTimings() {
        return ProfilerVulkanGpuTimings();
    }

    const std::string& GetZoneCPUTimings() {
        return ProfilerVulkanCpuTimings();
    }

    const std::string& GetTotalGPUTime() {
        return ProfilerVulkanTotalGPU();
    }

    const std::string& GetTotalCPUTime() {
        return ProfilerVulkanTotalCPU();
    }

    bool BeginSwapchainFrame(SwapchainFrame& frame) {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        std::vector<VkImage>& swapchainImages = VulkanSwapchainManager::GetSwapchainImages();
        std::vector<VkImageView>& swapchainImageViews = VulkanSwapchainManager::GetSwapchainImageViews();

        if (g_swapchainImageLayouts.size() != swapchainImages.size()) {
            g_swapchainImageLayouts.assign(swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
        }

        frame.frameIndex = g_frameIndex % FRAME_OVERLAP;

        VkResult waitResult = VulkanSyncManager::WaitForRenderFence(frame.frameIndex);
        if (waitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed while waiting for render fence: " << static_cast<int>(waitResult) << "\n";
            return false;
        }

        VulkanDeletionQueue::Flush(frame.frameIndex);
        VulkanDeletionQueue::SetFrameIndex(frame.frameIndex);

        VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VulkanSyncManager::GetPresentSemaphore(frame.frameIndex), VK_NULL_HANDLE, &frame.imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            CreatePresentRenderTarget(VulkanSwapchainManager::GetSwapchainExtent());
            return false;
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to acquire swapchain image\n";
            return false;
        }

        frame.extent = VulkanSwapchainManager::GetSwapchainExtent();
        frame.swapchainImage = swapchainImages[frame.imageIndex];
        frame.swapchainImageView = swapchainImageViews[frame.imageIndex];
        frame.presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        if (!frame.presentImage) return false;

        VkResult resetFenceResult = VulkanSyncManager::ResetRenderFence(frame.frameIndex);
        if (resetFenceResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to reset render fence: " << static_cast<int>(resetFenceResult) << "\n";
            return false;
        }

        VkCommandPool commandPool = VulkanCommandManager::GetGraphicsCommandPool(frame.frameIndex);
        frame.commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frame.frameIndex);

        VkResult resetCommandPoolResult = vkResetCommandPool(device, commandPool, 0);
        if (resetCommandPoolResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to reset command pool: " << static_cast<int>(resetCommandPoolResult) << "\n";
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkResult beginCommandBufferResult = vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
        if (beginCommandBufferResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to begin command buffer: " << static_cast<int>(beginCommandBufferResult) << "\n";
            return false;
        }

        ProfilerVulkanBeginFrame(frame.commandBuffer, frame.frameIndex);

        ResetDrawCommandOffset();

        vktools::setImageLayout(frame.commandBuffer, frame.swapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, g_swapchainImageLayouts[frame.imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_swapchainImageLayouts[frame.imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        return true;
    }

    void EndSwapchainFrame(SwapchainFrame& frame) {
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();

        vktools::setImageLayout(frame.commandBuffer, frame.swapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        g_swapchainImageLayouts[frame.imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        ProfilerVulkanEndFrame(frame.commandBuffer);

        VkResult endCommandBufferResult = vkEndCommandBuffer(frame.commandBuffer);
        if (endCommandBufferResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to end command buffer: " << static_cast<int>(endCommandBufferResult) << "\n";
            return;
        }

        VkSemaphore waitSemaphore = VulkanSyncManager::GetPresentSemaphore(frame.frameIndex);
        VkSemaphore signalSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frame.frameIndex, frame.imageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &frame.commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        VkResult submitResult = vkQueueSubmit(VulkanDeviceManager::GetGraphicsQueue(), 1, &submitInfo, VulkanSyncManager::GetRenderFence(frame.frameIndex));
        if (submitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to submit command buffer: " << static_cast<int>(submitResult) << "\n";
            return;
        }

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &frame.imageIndex;

        VkResult presentResult = vkQueuePresentKHR(VulkanDeviceManager::GetPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            CreatePresentRenderTarget(VulkanSwapchainManager::GetSwapchainExtent());
        }
        else if (presentResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to present swapchain image\n";
        }

        g_frameIndex = (g_frameIndex + 1) % FRAME_OVERLAP;
    }

}
