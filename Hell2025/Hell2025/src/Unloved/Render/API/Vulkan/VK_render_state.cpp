#include "VK_render_state.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include <array>

namespace VulkanRenderer {

    RenderTargetInfo& VulkanRenderState::AddColorTarget(const std::string& imageName) {
        if (colorTargetCount >= MAX_RENDER_TARGET_COUNT) {
            return colorTargets[MAX_RENDER_TARGET_COUNT - 1];
        }

        RenderTargetInfo& target = colorTargets[colorTargetCount++];
        target = RenderTargetInfo();
        target.imageName = imageName;
        return target;
    }

    RenderTargetInfo& VulkanRenderState::SetDepthTarget(const std::string& imageName) {
        hasDepthTarget = true;
        depthTarget = RenderTargetInfo();
        depthTarget.imageName = imageName;
        return depthTarget;
    }

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent) {
        if (state.colorTargetCount == 0 && !state.hasDepthTarget) return false;

        std::array<VkRenderingAttachmentInfo, MAX_RENDER_TARGET_COUNT> colorAttachments{};

        for (uint32_t i = 0; i < state.colorTargetCount; i++) {
            const RenderTargetInfo& target = state.colorTargets[i];
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(target.imageName);
            if (!image) return false;

            VkRenderingAttachmentInfo& attachment = colorAttachments[i];
            attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachment.imageView = image->GetImageView();
            attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.loadOp = target.loadOp;
            attachment.storeOp = target.storeOp;
            attachment.clearValue = target.clearValue;
        }

        VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

        if (state.hasDepthTarget) {
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(state.depthTarget.imageName);
            if (!image) return false;

            depthAttachment.imageView = image->GetImageView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = state.depthTarget.loadOp;
            depthAttachment.storeOp = state.depthTarget.storeOp;
            depthAttachment.clearValue = state.depthTarget.clearValue;
        }

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = state.colorTargetCount;
        renderingInfo.pColorAttachments = state.colorTargetCount ? colorAttachments.data() : nullptr;
        renderingInfo.pDepthAttachment = state.hasDepthTarget ? &depthAttachment : nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        return true;
    }

    void EndRenderState(VkCommandBuffer commandBuffer) {
        vkCmdEndRendering(commandBuffer);
    }

    bool ApplyRenderStateToPipeline(VulkanPipeline& pipeline, const VulkanRenderState& state) {
        for (uint32_t i = 0; i < state.colorTargetCount; i++) {
            const RenderTargetInfo& target = state.colorTargets[i];
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
        pipeline.SetFrontFace(state.rasterizer.frontFace);
        pipeline.SetCullMode(state.rasterizer.cullFaceEnabled ? state.rasterizer.cullMode : VK_CULL_MODE_NONE);
        pipeline.SetColorBlending(state.rasterizer.blendEnabled);
        return true;
    }
}
