#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

using namespace Unloved;

namespace VulkanRenderer {
    namespace {

    void RenderVisibilityPassOpaque(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Visibility");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Visibility");
        VulkanPipeline* skinnedPipeline = VulkanResourceManager::GetPipeline("VisibilitySkinned");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!assetGeometry) return;
        if (!proceduralGeometry) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!skinnedPipeline) return;
        if (!skinnedVertexBuffer) return;

        std::array<VulkanDrawCommandBatch, 4> standardCommands = WriteDrawCommandsByViewport(drawInfoSet.standard);
        std::array<VulkanDrawCommandBatch, 4> proceduralCommands = WriteDrawCommandsByViewport(drawInfoSet.procedural);
        std::array<VulkanDrawCommandBatch, 4> skinnedStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedStandard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingStandard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
                pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
                pushConstants.skinnedVerticesDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                assetGeometry->Bind(commandBuffer);
                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_STATIC);
                MultiDrawIndexedCommands(commandBuffer, standardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingStandardCommands[i]);

                proceduralGeometry->Bind(commandBuffer);
                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_PROCEDUAL);
                MultiDrawIndexedCommands(commandBuffer, proceduralCommands[i]);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedPipeline->GetHandle());
                vkCmdPushConstants(commandBuffer, skinnedPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                assetGeometry->Bind(commandBuffer);
                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_SKINNED);
                MultiDrawIndexedCommands(commandBuffer, skinnedStandardCommands[i]);
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
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("VisibilityAlphaDiscard");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityAlphaDiscard");
        VulkanPipeline* skinnedPipeline = VulkanResourceManager::GetPipeline("VisibilitySkinnedAlphaDiscard");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!skinnedPipeline) return;
        if (!skinnedVertexBuffer) return;

        std::array<VulkanDrawCommandBatch, 4> alphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.alphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> hairCommands = WriteDrawCommandsByViewport(drawInfoSet.hair);
        std::array<VulkanDrawCommandBatch, 4> skinnedAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedAlphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedHair);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingAlphaDiscard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        meshBuffer->Bind(commandBuffer);

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
                pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
                pushConstants.skinnedVerticesDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_STATIC);
                MultiDrawIndexedCommands(commandBuffer, alphaDiscardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingAlphaDiscardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, hairCommands[i]);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedPipeline->GetHandle());
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
                vkCmdPushConstants(commandBuffer, skinnedPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_SKINNED);
                MultiDrawIndexedCommands(commandBuffer, skinnedAlphaDiscardCommands[i]);
                vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_SKINNED_HAIR);
                MultiDrawIndexedCommands(commandBuffer, skinnedHairCommands[i]);
            }
        }

        EndRenderState(commandBuffer);
    }

    }

    void VisibilityPass(VkCommandBuffer commandBuffer) {
        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Depth");
        if (!visibilityImage || !depthImage) return;

        VkExtent2D extent = visibilityImage->GetExtent2D();

        RenderVisibilityPassOpaque(commandBuffer, extent);
        RenderVisibilityAlphaDiscardPass(commandBuffer, extent);

        visibilityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }

    void DebugPass(VkCommandBuffer commandBuffer) {
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityDebug");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet || !lightingImage) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkClearValue colorClear{};
        colorClear.color.float32[3] = 1.0f;

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = lightingImage->GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
