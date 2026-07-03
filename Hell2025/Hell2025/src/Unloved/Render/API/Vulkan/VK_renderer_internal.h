#pragma once
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Hell/Render/API/Vulkan/Types/VK_render_state.h"

#include <string>
#include <vector>

struct AllocatedImage;
struct VulkanBuffer;
struct VulkanPipeline;

namespace VulkanRenderer {
    struct SwapchainFrame {
        uint32_t frameIndex = 0;
        uint32_t imageIndex = 0;
        VkExtent2D extent = {};
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkImage swapchainImage = VK_NULL_HANDLE;
        VkImageView swapchainImageView = VK_NULL_HANDLE;
        AllocatedImage* presentImage = nullptr;
    };

    extern uint32_t g_frameIndex;
    extern std::vector<VkImageLayout> g_swapchainImageLayouts;
    extern bool g_staticSamplersUploaded;

    void CreatePipelines();
    void CreateRenderStates();
    void CreateRenderTargets();
    void CreatePresentRenderTarget(VkExtent2D extent);
    void CreateShaders();

    void UpdateBindlessRenderTargetDescriptors();

    void UpdateBuffers();
    void UpdateBuffersUI();

    bool UpdateBuffer(VulkanBuffer* buffer, const void* data, VkDeviceSize size);
    bool EnsureBufferSize(uint64_t id, VkDeviceSize size);

    bool BeginSwapchainFrame(SwapchainFrame& frame);
    void EndSwapchainFrame(SwapchainFrame& frame);
    void BlitImage(VkCommandBuffer commandBuffer, const std::string& srcName, const std::string& dstName, VkFilter filter);

    void ComputeSkinningPass(VkCommandBuffer commandBuffer);
    void ComputeRedTestPass(VkCommandBuffer commandBuffer);

    void RenderLoadingScreenPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);
    void VisibilityPass(VkCommandBuffer commandBuffer);
    void MaterialResolvePass(VkCommandBuffer commandBuffer);
    void DebugPass(VkCommandBuffer commandBuffer);
    void RenderPresentPass(VkCommandBuffer commandBuffer, VkImageView imageView);

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent);
    void EndRenderState(VkCommandBuffer commandBuffer);
    bool ApplyRenderStateToPipeline(VulkanPipeline& pipeline, const VulkanRenderState& state);
}
