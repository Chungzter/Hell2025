#include "Unloved/Render/API/Vulkan/VK_draw.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Viewport/Viewport.h"

namespace VulkanRenderer {
    static VkDeviceSize g_drawCommandBufferOffset = 0;

    void BeginDrawCommandWrite() {
        // Reset shared draw command buffer offset
        g_drawCommandBufferOffset = 0;
    }

    VulkanDrawCommandBatch WriteDrawCommands(const std::vector<DrawIndexedIndirectCommand>& drawCommands) {
        VulkanDrawCommandBatch batch;

        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.drawCommands);
        if (!drawCommandBuffer || drawCommands.empty()) return batch;

        VkDeviceSize writeSize = sizeof(DrawIndexedIndirectCommand) * drawCommands.size();
        VkDeviceSize offset = g_drawCommandBufferOffset;

        // Bail if buffer is full
        if (offset + writeSize > drawCommandBuffer->GetSize()) {
            Logging::Error() << "VulkanRenderer::WriteDrawCommands() draw command buffer capacity exceeded\n";
            return batch;
        }

        batch.offset = offset;
        batch.count = static_cast<uint32_t>(drawCommands.size());

        // Write commands into shared indirect buffer
        drawCommandBuffer->UpdateData(drawCommands.data(), writeSize, offset);
        g_drawCommandBufferOffset += writeSize;
        return batch;
    }

    std::array<VulkanDrawCommandBatch, 4> WriteDrawCommandsByViewport(const std::vector<DrawIndexedIndirectCommand> drawCommands[4]) {
        std::array<VulkanDrawCommandBatch, 4> batches;
        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            batches[viewportIndex] = WriteDrawCommands(drawCommands[viewportIndex]);
        }
        return batches;
    }

    void DrawIndexedCommands(VkCommandBuffer commandBuffer, const std::vector<DrawIndexedIndirectCommand>& drawCommands) {
        for (const DrawIndexedIndirectCommand& command : drawCommands) {
            vkCmdDrawIndexed(commandBuffer, command.indexCount, command.instanceCount, command.firstIndex, command.baseVertex, command.baseInstance);
        }
    }

    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, const VulkanDrawCommandBatch& batch) {
        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.drawCommands);
        if (!drawCommandBuffer) return;

        MultiDrawIndexedCommands(commandBuffer, *drawCommandBuffer, batch.offset, batch.count);
    }

    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, VulkanBuffer& drawCommandBuffer, VkDeviceSize drawCommandOffset, uint32_t drawCount) {
        if (drawCount == 0) return;

        vkCmdDrawIndexedIndirect(commandBuffer, drawCommandBuffer.GetBuffer(), drawCommandOffset, drawCount, sizeof(DrawIndexedIndirectCommand));
    }

    void SetGameViewportAndScissor(VkCommandBuffer commandBuffer, const Unloved::Viewport& viewport, VkExtent2D extent) {
        glm::vec2 pos = viewport.GetPosition();
        glm::vec2 size = viewport.GetSize();

        int32_t x = static_cast<int32_t>(pos.x * extent.width);
        int32_t y = static_cast<int32_t>(pos.y * extent.height);
        uint32_t width = static_cast<uint32_t>(size.x * extent.width);
        uint32_t height = static_cast<uint32_t>(size.y * extent.height);

        VkViewport vkViewport{};
        vkViewport.x = static_cast<float>(x);
        vkViewport.y = static_cast<float>(y);
        vkViewport.width = static_cast<float>(width);
        vkViewport.height = static_cast<float>(height);
        vkViewport.minDepth = 0.0f;
        vkViewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);

        VkRect2D scissor{};
        scissor.offset.x = x;
        scissor.offset.y = y;
        scissor.extent.width = width;
        scissor.extent.height = height;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
}
