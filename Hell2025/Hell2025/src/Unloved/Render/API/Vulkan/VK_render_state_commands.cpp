#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include <array>

namespace VulkanRenderer {

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent) {
        if (state.colorTargetCount == 0 && !state.hasDepthTarget) return false;

        std::array<VkRenderingAttachmentInfo, VulkanRenderState::MAX_RENDER_TARGET_COUNT> colorAttachments{};

        for (uint32_t i = 0; i < state.colorTargetCount; i++) {
            const VulkanRenderTargetInfo& target = state.colorTargets[i];
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(target.imageName);
            if (!image) return false;

            image->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkRenderingAttachmentInfo& attachment = colorAttachments[i];
            attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachment.imageView = image->GetImageView();
            attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            attachment.loadOp = target.loadOp;
            attachment.storeOp = target.storeOp;
            attachment.clearValue = target.clearValue;
        }

        VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

        if (state.hasDepthTarget) {
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(state.depthTarget.imageName);
            if (!image) return false;

            VkAccessFlags2 accessFlags = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            if (state.rasterizer.depthWriteEnabled || state.rasterizer.stencilWriteMask != 0) {
                accessFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            image->Sync(commandBuffer, accessFlags, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);

            depthAttachment.imageView = image->GetImageView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            depthAttachment.loadOp = state.depthTarget.loadOp;
            depthAttachment.storeOp = state.depthTarget.storeOp;
            depthAttachment.clearValue = state.depthTarget.clearValue;
        }

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = state.colorTargetCount;
        renderingInfo.pColorAttachments = state.colorTargetCount ? colorAttachments.data() : nullptr;
        bool useDepthAttachment = state.hasDepthTarget && (state.rasterizer.depthTestEnabled || state.rasterizer.depthWriteEnabled);
        bool useStencilAttachment = state.hasDepthTarget && state.rasterizer.stencilTestEnabled;

        renderingInfo.pDepthAttachment = useDepthAttachment ? &depthAttachment : nullptr;
        renderingInfo.pStencilAttachment = useStencilAttachment ? &depthAttachment : nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        return true;
    }

    void EndRenderState(VkCommandBuffer commandBuffer) {
        vkCmdEndRendering(commandBuffer);
    }

    bool ApplyRenderStateToPipeline(VulkanPipeline& pipeline, const VulkanRenderState& state) {
        for (uint32_t i = 0; i < state.colorTargetCount; i++) {
            const VulkanRenderTargetInfo& target = state.colorTargets[i];
            VkFormat format = target.format;

            if (format == VK_FORMAT_UNDEFINED) {
                AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(target.imageName);
                if (!image) return false;
                format = image->GetFormat();
            }

            pipeline.AddColorAttachmentFormat(format);
        }

        if (state.hasDepthTarget) {
            VkFormat format = state.depthTarget.format;

            if (format == VK_FORMAT_UNDEFINED) {
                AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(state.depthTarget.imageName);
                if (!image) return false;
                format = image->GetFormat();
            }

            pipeline.SetDepthAttachmentFormat(format);
        }

        pipeline.SetDepthTest(state.rasterizer.depthTestEnabled, state.rasterizer.depthWriteEnabled);
        pipeline.SetDepthCompareOp(state.rasterizer.depthCompareOp);
        pipeline.SetStencilTest(state.rasterizer.stencilTestEnabled, state.rasterizer.stencilCompareOp, state.rasterizer.stencilFailOp, state.rasterizer.stencilDepthFailOp, state.rasterizer.stencilPassOp, state.rasterizer.stencilReadMask, state.rasterizer.stencilWriteMask);
        pipeline.SetFrontFace(state.rasterizer.frontFace);
        pipeline.SetCullMode(state.rasterizer.cullFaceEnabled ? state.rasterizer.cullMode : VK_CULL_MODE_NONE);
        pipeline.SetColorBlending(state.rasterizer.blendEnabled);
        return true;
    }
}
