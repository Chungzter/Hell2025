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
        ProfilerVulkanZoneFunction();

        const std::vector<SkinningDispatchGroup>& skinningDispatchGroups = Unloved::RenderDataManager::GetSkinningDispatchGroups();
        if (skinningDispatchGroups.empty()) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();
        if (!EnsureBufferSize(frameData.buffers.skinnedVertices, sizeof(Vertex) * totalVertexCount)) return;

        VulkanBuffer* outputVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);
        VulkanBuffer* skinningDispatchGroupsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinningDispatchGroups);
        VulkanBuffer* skinningJobsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinningJobs);
        VulkanBuffer* skinningTransformsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinningTransforms);
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("ComputeSkinning");

        if (!outputVertexBuffer) return;
        if (!skinningDispatchGroupsBuffer) return;
        if (!skinningJobsBuffer) return;
        if (!skinningTransformsBuffer) return;
        if (!meshBuffer) return;
        if (!pipeline) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetVertexWeightBuffer()) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());

        PushConstantsSkinning pushConstants{};
        pushConstants.outputVerticesDeviceAddress = outputVertexBuffer->GetDeviceAddress();
        pushConstants.inputVerticesDeviceAddress = meshBuffer->GetVertexBufferAddress();
        pushConstants.vertexWeightsDeviceAddress = meshBuffer->GetVertexWeightBufferAddress();
        pushConstants.skinningDispatchGroupsDeviceAddress = skinningDispatchGroupsBuffer->GetDeviceAddress();
        pushConstants.skinningJobsDeviceAddress = skinningJobsBuffer->GetDeviceAddress();
        pushConstants.skinningTransformsDeviceAddress = skinningTransformsBuffer->GetDeviceAddress();

        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantsSkinning), &pushConstants);
        
        vkCmdDispatch(commandBuffer, static_cast<uint32_t>(skinningDispatchGroups.size()), 1, 1);

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = outputVertexBuffer->GetBuffer();
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }
}
