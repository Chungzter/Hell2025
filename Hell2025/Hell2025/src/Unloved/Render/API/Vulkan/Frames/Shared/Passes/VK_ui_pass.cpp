#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Unloved/Render/RenderDataManager.h"

namespace VulkanRenderer {

    void RenderUIPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI();
        if (!presentImage) return;
        if (renderItems.empty() || drawCommands.empty()) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        VkExtent2D extent = presentImage->GetExtent2D();
        presentImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        VulkanGenericMesh* vulkanMesh = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.ui);
        if (!vulkanMesh || vulkanMesh->GetIndexCount() == 0) return;
        VulkanBuffer* vertexBuffer = vulkanMesh->GetVertexBuffer();
        VulkanBuffer* indexBuffer = vulkanMesh->GetIndexBuffer();
        if (!vertexBuffer || !indexBuffer) return;

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("UI");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet) return;

        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems);
        if (!renderItemBuffer) return;

        VulkanDrawCommandBatch drawCommandBatch = WriteDrawCommands(drawCommands);
        if (drawCommandBatch.count == 0) return;

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = presentImage->GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

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

        PushConstantsUI pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.renderTargetWidth = static_cast<float>(Config::GetResolutions().ui.x);
        pushConstants.renderTargetHeight = static_cast<float>(Config::GetResolutions().ui.y);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        BindVertexBuffer(commandBuffer, vertexBuffer);
        BindIndexBuffer(commandBuffer, indexBuffer);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
        MultiDrawIndexedCommands(commandBuffer, drawCommandBatch);

        vkCmdEndRendering(commandBuffer);
    }
}
