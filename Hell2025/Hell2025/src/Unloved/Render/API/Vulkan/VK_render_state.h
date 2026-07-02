#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"

#include <string>

struct VulkanPipeline;

namespace VulkanRenderer {
    inline constexpr uint32_t MAX_RENDER_TARGET_COUNT = 8;

    struct RenderTargetInfo {
        std::string imageName = "";
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkClearValue clearValue{};
    };

    struct RasterizerState {
        bool depthTestEnabled = false;
        bool depthWriteEnabled = false;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;

        bool blendEnabled = false;
        bool cullFaceEnabled = false;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        bool stencilTestEnabled = false;
        uint32_t stencilRef = 0;
        uint32_t stencilReadMask = 0xff;
        uint32_t stencilWriteMask = 0xff;
    };

    struct VulkanRenderState {
        RenderTargetInfo colorTargets[MAX_RENDER_TARGET_COUNT];
        uint32_t colorTargetCount = 0;
        RenderTargetInfo depthTarget;
        bool hasDepthTarget = false;
        RasterizerState rasterizer;

        RenderTargetInfo& AddColorTarget(const std::string& imageName = "");
        RenderTargetInfo& SetDepthTarget(const std::string& imageName = "");
    };

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent);
    void EndRenderState(VkCommandBuffer commandBuffer);
    bool ApplyRenderStateToPipeline(VulkanPipeline& pipeline, const VulkanRenderState& state);
}
