#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"

namespace VulkanRenderer {

    void ComputeSkinningPass(VkCommandBuffer commandBuffer) {
        const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetCombinedSkinnedRenderItems();
        if (renderItems.empty()) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();
        if (!EnsureBufferSize(frameData.buffers.skinnedVertices, sizeof(Vertex) * totalVertexCount)) return;

        VulkanBuffer* outputVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);
        VulkanBuffer* skinningTransformsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinningTransforms);
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("ComputeSkinning");

        if (!outputVertexBuffer) return;
        if (!skinningTransformsBuffer) return;
        if (!meshBuffer) return;
        if (!pipeline) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetVertexWeightBuffer()) return;

        Hell::MeshBuffer& assetGeometry = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());

        PushConstantsSkinning pushConstants{};
        pushConstants.outputVerticesDeviceAddress = outputVertexBuffer->GetDeviceAddress();
        pushConstants.inputVerticesDeviceAddress = meshBuffer->GetVertexBufferAddress();
        pushConstants.skinningTransformsDeviceAddress = skinningTransformsBuffer->GetDeviceAddress();
        pushConstants.vertexWeightsDeviceAddress = meshBuffer->GetVertexWeightBufferAddress();

        for (const RenderItem& renderItem : renderItems) {
            Mesh* mesh = assetGeometry.GetMeshById(renderItem.meshId);
            if (!mesh) continue;

            pushConstants.vertexCount = mesh->vertexCount;
            pushConstants.baseInputVertex = mesh->baseVertex;
            pushConstants.baseInputVertexWeight = renderItem.baseVertexWeight;
            pushConstants.baseOutputVertex = renderItem.baseVertex;
            pushConstants.baseTransformIndex = renderItem.baseSkinningTransformIndex;

            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantsSkinning), &pushConstants);

            uint32_t workGroupSize = 128;
            uint32_t groupCountX = (mesh->vertexCount + workGroupSize - 1) / workGroupSize;
            vkCmdDispatch(commandBuffer, groupCountX, 1, 1);
        }

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = outputVertexBuffer->GetBuffer();
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }
}
