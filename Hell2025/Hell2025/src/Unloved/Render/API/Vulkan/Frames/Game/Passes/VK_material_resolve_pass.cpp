#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RendererConstants.h"

namespace VulkanRenderer {
    namespace {

    void RenderMaterialResolvePasses(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("MaterialResolve");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("MaterialResolve");

        if (!renderItemBuffer) return;
        if (!viewportDataBuffer) return;
        if (!rendererDataBuffer) return;
        if (!staticDescriptorSet) return;
        if (!assetGeometry) return;
        if (!proceduralGeometry) return;
        if (!skinnedVertexBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!assetGeometry->GetVertexBuffer()) return;
        if (!assetGeometry->GetIndexBuffer()) return;
        if (!proceduralGeometry->GetVertexBuffer()) return;
        if (!proceduralGeometry->GetIndexBuffer()) return;

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        PushConstantsMaterialResolve pushConstants{};
        pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();
        pushConstants.viewportDataDeviceAddress = viewportDataBuffer->GetDeviceAddress();
        pushConstants.rendererDataDeviceAddress = rendererDataBuffer->GetDeviceAddress();

        pushConstants.vertexBufferDeviceAddress = assetGeometry->GetVertexBufferAddress();
        pushConstants.indexBufferDeviceAddress = assetGeometry->GetIndexBufferAddress();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsMaterialResolve), &pushConstants);
        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_STATIC);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        //pushConstants.vertexBufferDeviceAddress = proceduralGeometry->GetVertexBufferAddress();
        //pushConstants.indexBufferDeviceAddress = proceduralGeometry->GetIndexBufferAddress();
        //vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsMaterialResolve), &pushConstants);
        //vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_PROCEDUAL);
        //vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        //
        //pushConstants.vertexBufferDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
        //pushConstants.indexBufferDeviceAddress = assetGeometry->GetIndexBufferAddress();
        //vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsMaterialResolve), &pushConstants);
        //vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, STENCIL_BIT_SKINNED);
        //vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        EndRenderState(commandBuffer);
    }

    }

    void MaterialResolvePass(VkCommandBuffer commandBuffer) {
        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* velocityImage = VulkanResourceManager::GetAllocatedImage("VelocityXYOcclusionSubSurface");
        if (!baseColorImage || !normalImage || !velocityImage) return;

        VkExtent2D extent = baseColorImage->GetExtent2D();

        RenderMaterialResolvePasses(commandBuffer, extent);

        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        velocityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}
