#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

using namespace Unloved;

namespace VulkanRenderer {
    void LightingForwardBlendedPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("LightingForwardBlended");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("LightingForwardBlended");
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

        if (!lightingImage) return;
        if (!depthImage) return;
        if (!pipeline) return;
        if (!renderState) return;
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

        std::array<VulkanDrawCommandBatch, 4> blendedCommands = WriteDrawCommandsByViewport(drawInfoSet.blended);
        std::array<VulkanDrawCommandBatch, 4> skinnedBlendedCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedBlended);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingBlendedCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingBlended);

        VkExtent2D extent = lightingImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle(), rayQueryDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);

        PushConstantsDeferredLighting pushConstants{};
        pushConstants.frame = CreatePushConstantsFrameResources();
        pushConstants.rayQueryBLASInstanceDataDeviceAddress = rayQueryBLASInstanceDataBuffer->GetDeviceAddress();
        pushConstants.rayQueryMeshInstanceDataDeviceAddress = rayQueryMeshInstanceDataBuffer->GetDeviceAddress();
        pushConstants.rayQueryEnabled = 1;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsDeferredLighting), &pushConstants);

        BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        // Static and Procedural
        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);

            MultiDrawIndexedCommands(commandBuffer, blendedCommands[i]);
            MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingBlendedCommands[i]);
        }

        // Skinned
        BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);

            MultiDrawIndexedCommands(commandBuffer, skinnedBlendedCommands[i]);
        }
       
        EndRenderState(commandBuffer);
    }
}
