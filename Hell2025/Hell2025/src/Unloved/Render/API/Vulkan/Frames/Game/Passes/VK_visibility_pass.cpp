#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/API/Vulkan/vk_render_states.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

using namespace Unloved;

namespace VulkanRenderer {

    void RenderVisibilityPass(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanRenderState* renderState = VulkanRenderer::GetRenderState("Visibility");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Visibility");

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!meshBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;

        std::array<VulkanDrawCommandBatch, 4> standardDrawCommandBatches = WriteDrawCommandsByViewport(drawInfoSet.standard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingStandardDrawCommandBatches = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingStandard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        meshBuffer->Bind(commandBuffer);

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
                pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                MultiDrawIndexedCommands(commandBuffer, standardDrawCommandBatches[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingStandardDrawCommandBatches[i]);
            }
        }

        EndRenderState(commandBuffer);
    }

    void RenderVisibilityAlphaDiscardPass(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanRenderState* renderState = VulkanRenderer::GetRenderState("VisibilityAlphaDiscard");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityAlphaDiscard");

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;

        std::array<VulkanDrawCommandBatch, 4> alphaDiscardDrawCommandBatches = WriteDrawCommandsByViewport(drawInfoSet.alphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingAlphaDiscardDrawCommandBatches = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingAlphaDiscard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        meshBuffer->Bind(commandBuffer);

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
                pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                MultiDrawIndexedCommands(commandBuffer, alphaDiscardDrawCommandBatches[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingAlphaDiscardDrawCommandBatches[i]);
            }
        }

        EndRenderState(commandBuffer);
    }

    void RenderVisibilityDebugPass(VkCommandBuffer commandBuffer, VkImageView lightingImageView, VkExtent2D extent) {
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityDebug");
        VulkanDescriptorSet* descriptorSet = VulkanResourceManager::GetDescriptorSet("VisibilityDebugDescriptorSet");
        if (!pipeline || !descriptorSet) return;

        VkClearValue colorClear{};
        colorClear.color.float32[3] = 1.0f;

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = lightingImageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = colorClear;

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, descriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
