#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"

namespace VulkanRenderer {

    void RenderUIPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent) {
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI();
        if (renderItems.empty() || drawCommands.empty()) return;

        Hell::GenericMesh& genericMesh = Hell::ResourceManager::GetGenericMesh("UI");
        if (genericMesh.GetVulkanId() == 0 || genericMesh.GetIndexCount() == 0) return;

        VulkanGenericMesh* vulkanMesh = VulkanResourceManager::GetGenericMesh(genericMesh.GetVulkanId());
        if (!vulkanMesh) return;

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("UI");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems);
        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiDrawCommands);
        if (!renderItemBuffer || !drawCommandBuffer) return;

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
        pushConstants.renderItemsDeviceAddress = renderItemBuffer->GetDeviceAddress();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vulkanMesh->Bind(commandBuffer);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsUI), &pushConstants);
        vkCmdDrawIndexedIndirect(commandBuffer, drawCommandBuffer->GetBuffer(), 0, static_cast<uint32_t>(drawCommands.size()), sizeof(DrawIndexedIndirectCommand));

        vkCmdEndRendering(commandBuffer);
    }
}
