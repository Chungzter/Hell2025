#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

using namespace Unloved;

namespace VulkanRenderer {
    void HairDepthPrep(VkCommandBuffer commandBuffer);
    void HairDepthPrePass(VkCommandBuffer commandBuffer);
    void HairForwardLightingPass(VkCommandBuffer commandBuffer);
    void HairComposite(VkCommandBuffer commandBuffer);

    void HairPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        HairDepthPrep(commandBuffer);
        HairDepthPrePass(commandBuffer);
        HairForwardLightingPass(commandBuffer);
        HairComposite(commandBuffer);
    }

    namespace {
        void SetFullscreenViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent) {
            VkViewport viewport{};
            viewport.width = static_cast<float>(extent.width);
            viewport.height = static_cast<float>(extent.height);
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }

        PushConstantsVisibility CreateVisibilityPushConstants(VulkanBuffer* renderItemBuffer, VulkanBuffer* viewportDataBuffer, VulkanBuffer* materialsBuffer, VulkanBuffer* skinnedVertexBuffer) {
            PushConstantsVisibility pushConstants{};
            pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
            pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
            pushConstants.skinnedVerticesDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
            pushConstants.materialsDeviceAddress = materialsBuffer->GetDeviceAddress();
            return pushConstants;
        }

        PushConstantsHair CreateHairPushConstants(VulkanBuffer* rayQueryBLASInstanceDataBuffer, VulkanBuffer* rayQueryMeshInstanceDataBuffer) {
            PushConstantsHair pushConstants{};
            pushConstants.frame = CreatePushConstantsFrameResources();
            pushConstants.rayQueryBLASInstanceDataDeviceAddress = rayQueryBLASInstanceDataBuffer->GetDeviceAddress();
            pushConstants.rayQueryMeshInstanceDataDeviceAddress = rayQueryMeshInstanceDataBuffer->GetDeviceAddress();
            pushConstants.flashlightCookieTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("Flashlight2", true);
            pushConstants.rayQueryEnabled = 1;
            return pushConstants;
        }

        void DrawStaticAndNonDeformingHair(VkCommandBuffer commandBuffer, VulkanMeshBuffer* meshBuffer, const std::array<VulkanDrawCommandBatch, 4>& hairCommands, const std::array<VulkanDrawCommandBatch, 4>& skinnedNonDeformingHairCommands, VkExtent2D extent) {
            BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
            BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

            for (uint32_t i = 0; i < 4; i++) {
                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);
                MultiDrawIndexedCommands(commandBuffer, hairCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingHairCommands[i]);
            }
        }

        void DrawSkinnedHair(VkCommandBuffer commandBuffer, VulkanMeshBuffer* meshBuffer, VulkanBuffer* skinnedVertexBuffer, const std::array<VulkanDrawCommandBatch, 4>& skinnedHairCommands, VkExtent2D extent) {
            BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
            BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

            for (uint32_t i = 0; i < 4; i++) {
                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);
                MultiDrawIndexedCommands(commandBuffer, skinnedHairCommands[i]);
            }
        }
    }

    void HairDepthPrep(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* hairLightingImage = VulkanResourceManager::GetAllocatedImage("HairLighting");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("HairDepthPrep");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("HairDepthPrep");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!depthImage) return;
        if (!hairLightingImage) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;

        VkExtent2D extent = hairLightingImage->GetExtent2D();
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        SetFullscreenViewportAndScissor(commandBuffer, extent);
        SetStencilReference(commandBuffer, 0);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        EndRenderState(commandBuffer);
    }

    void HairDepthPrePass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* hairDepthImage = VulkanResourceManager::GetAllocatedImage("HairDepth");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("HairDepthPrePass");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("HairDepthPrePass");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!hairDepthImage) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!materialsBuffer) return;
        if (!skinnedVertexBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> hairCommands = WriteDrawCommandsByViewport(drawInfoSet.hair);
        std::array<VulkanDrawCommandBatch, 4> skinnedHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedHair);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingHair);

        VkExtent2D extent = hairDepthImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        PushConstantsVisibility pushConstants = CreateVisibilityPushConstants(renderItemBuffer, viewportDataBuffer, materialsBuffer, skinnedVertexBuffer);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsVisibility), &pushConstants);

        DrawStaticAndNonDeformingHair(commandBuffer, meshBuffer, hairCommands, skinnedNonDeformingHairCommands, extent);
        DrawSkinnedHair(commandBuffer, meshBuffer, skinnedVertexBuffer, skinnedHairCommands, extent);

        EndRenderState(commandBuffer);
    }

    void HairForwardLightingPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* hairLightingImage = VulkanResourceManager::GetAllocatedImage("HairLighting");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("HairLighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("HairLighting");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSetResource* rayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        VulkanDescriptorSet* rayQueryDescriptorSet = rayQueryDescriptorSetResource ? &rayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VulkanBuffer* gpuLightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.lights);
        VulkanBuffer* rayQueryBLASInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASInstanceData);
        VulkanBuffer* rayQueryMeshInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryMeshInstanceData);
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!hairLightingImage) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryDescriptorSet) return;
        if (!meshBuffer) return;
        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!rendererDataBuffer) return;
        if (!materialsBuffer) return;
        if (!gpuLightsBuffer) return;
        if (!rayQueryBLASInstanceDataBuffer) return;
        if (!rayQueryMeshInstanceDataBuffer) return;
        if (!skinnedVertexBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> hairCommands = WriteDrawCommandsByViewport(drawInfoSet.hair);
        std::array<VulkanDrawCommandBatch, 4> skinnedHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedHair);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingHair);

        VkExtent2D extent = hairLightingImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        PushConstantsHair pushConstants = CreateHairPushConstants(rayQueryBLASInstanceDataBuffer, rayQueryMeshInstanceDataBuffer);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle(), rayQueryDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsHair), &pushConstants);

        DrawSkinnedHair(commandBuffer, meshBuffer, skinnedVertexBuffer, skinnedHairCommands, extent);
        DrawStaticAndNonDeformingHair(commandBuffer, meshBuffer, hairCommands, skinnedNonDeformingHairCommands, extent);

        EndRenderState(commandBuffer);
    }

    void HairComposite(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* hairLightingImage = VulkanResourceManager::GetAllocatedImage("HairLighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("HairComposite");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!lightingImage) return;
        if (!hairLightingImage) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;

        hairLightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        VkExtent2D extent = lightingImage->GetExtent2D();
        uint32_t groupCountX = (extent.width + 23) / 24;
        uint32_t groupCountY = (extent.height + 23) / 24;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    }
}
