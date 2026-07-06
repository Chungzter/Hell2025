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
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Visibility");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Visibility");
        VulkanPipeline* skinnedPipeline = VulkanResourceManager::GetPipeline("VisibilitySkinned");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!materialsBuffer) return;
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
                pushConstants.materialsDeviceAddress = materialsBuffer->GetDeviceAddress();
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
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("VisibilityAlphaDiscard");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityAlphaDiscard");
        VulkanPipeline* skinnedPipeline = VulkanResourceManager::GetPipeline("VisibilitySkinnedAlphaDiscard");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!materialsBuffer) return;
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
                pushConstants.materialsDeviceAddress = materialsBuffer->GetDeviceAddress();
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
        ProfilerVulkanZoneFunction();

        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Depth");
        if (!visibilityImage || !depthImage) return;

        VkExtent2D extent = visibilityImage->GetExtent2D();

        RenderVisibilityPassOpaque(commandBuffer, extent);
        RenderVisibilityAlphaDiscardPass(commandBuffer, extent);

        visibilityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}
