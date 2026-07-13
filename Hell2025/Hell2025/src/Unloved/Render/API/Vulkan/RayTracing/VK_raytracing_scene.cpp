#include "VK_raytracing_scene.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <array>

namespace VulkanRenderer {

    void RayQueryScene::Clear() {
        m_instances.clear();
        m_blasInstanceData.clear();
        m_meshInstanceData.clear();
        m_instanceBuffer = nullptr;
        m_blasInstanceDataBuffer = nullptr;
        m_meshInstanceDataBuffer = nullptr;
    }

    void RayQueryScene::Reserve(size_t blasInstanceCount, size_t meshInstanceDataCount) {
        m_instances.reserve(blasInstanceCount);
        m_blasInstanceData.reserve(blasInstanceCount);
        m_meshInstanceData.reserve(meshInstanceDataCount);
    }

    void RayQueryScene::AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMeshInstance& meshInstance) {
        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_blasInstanceData.size());

        RayQueryBLASInstanceData& data = m_blasInstanceData.emplace_back();
        data.meshInstanceDataOffset = static_cast<uint32_t>(m_meshInstanceData.size());
        data.meshInstanceDataCount = 1;

        m_meshInstanceData.push_back(CreateMeshInstanceData(vertexBufferAddress, indexBufferAddress, meshInstance));
        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
    }

    void RayQueryScene::AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances) {
        if (meshInstances.empty()) return;

        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_blasInstanceData.size());

        RayQueryBLASInstanceData& data = m_blasInstanceData.emplace_back();
        data.meshInstanceDataOffset = static_cast<uint32_t>(m_meshInstanceData.size());
        data.meshInstanceDataCount = static_cast<uint32_t>(meshInstances.size());

        m_meshInstanceData.reserve(m_meshInstanceData.size() + meshInstances.size());
        for (const RayQueryMeshInstance& meshInstance : meshInstances) {
            m_meshInstanceData.push_back(CreateMeshInstanceData(vertexBufferAddress, indexBufferAddress, meshInstance));
        }

        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
    }

    bool RayQueryScene::HasInstances() const {
        return !m_instances.empty();
    }

    uint32_t RayQueryScene::GetInstanceCount() const {
        return static_cast<uint32_t>(m_instances.size());
    }

    VkDeviceSize RayQueryScene::GetTLASScratchSize(const VulkanFrameData& frameData) const {
        return static_cast<VkDeviceSize>(frameData.accelerationStructures.rayQueryTLASScratchSize);
    }

    bool RayQueryScene::Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData) {
        if (m_instances.empty()) return false;

        VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_instances.size();
        VkDeviceSize blasInstanceDataSize = sizeof(RayQueryBLASInstanceData) * m_blasInstanceData.size();
        VkDeviceSize meshInstanceDataSize = sizeof(RayQueryMeshInstanceData) * m_meshInstanceData.size();

        if (!EnsureBufferSize(frameData.buffers.rayQueryInstances, instanceBufferSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQueryBLASInstanceData, blasInstanceDataSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQueryMeshInstanceData, meshInstanceDataSize)) return false;

        m_instanceBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstances);
        m_blasInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASInstanceData);
        m_meshInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryMeshInstanceData);
        if (!m_instanceBuffer) return false;
        if (!m_blasInstanceDataBuffer) return false;
        if (!m_meshInstanceDataBuffer) return false;

        m_instanceBuffer->UpdateData(m_instances.data(), instanceBufferSize);
        m_blasInstanceDataBuffer->UpdateData(m_blasInstanceData.data(), blasInstanceDataSize);
        m_meshInstanceDataBuffer->UpdateData(m_meshInstanceData.data(), meshInstanceDataSize);

        VkBufferMemoryBarrier instanceUploadBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        instanceUploadBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        instanceUploadBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        instanceUploadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.buffer = m_instanceBuffer->GetBuffer();
        instanceUploadBarrier.offset = 0;
        instanceUploadBarrier.size = instanceBufferSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 0, nullptr, 1, &instanceUploadBarrier, 0, nullptr);

        std::array<VkBufferMemoryBarrier, 2> metadataUploadBarriers{};
        for (VkBufferMemoryBarrier& barrier : metadataUploadBarriers) {
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.offset = 0;
        }

        metadataUploadBarriers[0].buffer = m_blasInstanceDataBuffer->GetBuffer();
        metadataUploadBarriers[0].size = blasInstanceDataSize;
        metadataUploadBarriers[1].buffer = m_meshInstanceDataBuffer->GetBuffer();
        metadataUploadBarriers[1].size = meshInstanceDataSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(metadataUploadBarriers.size()), metadataUploadBarriers.data(), 0, nullptr);

        return true;
    }

    bool RayQueryScene::ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity) {
        if (!m_instanceBuffer) return false;
        if (frameData.accelerationStructures.rayQueryTLAS == 0) return false;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryTopLevelBuildSize(m_instanceBuffer->GetDeviceAddress(), instanceCapacity);
        if (!PrepareAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizeInfo)) return false;

        frameData.accelerationStructures.rayQueryTLASInstanceCapacity = instanceCapacity;
        frameData.accelerationStructures.rayQueryTLASScratchSize = sizeInfo.buildScratchSize;
        return true;
    }

    void RayQueryScene::RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress) {
        if (!m_instanceBuffer) return;
        RecordTopLevelBuild(commandBuffer, frameData.accelerationStructures.rayQueryTLAS, m_instanceBuffer->GetDeviceAddress(), GetInstanceCount(), scratchBaseAddress);
    }

    bool RayQueryScene::BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding) {
        if (!descriptorSet) return false;

        VulkanAccelerationStructure* tlas = VulkanResourceManager::GetAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS);
        if (!tlas || tlas->GetHandle() == VK_NULL_HANDLE) return false;

        descriptorSet->WriteAccelerationStructure(binding, tlas->GetHandle());
        descriptorSet->Update();
        return true;
    }

    RayQueryMeshInstanceData RayQueryScene::CreateMeshInstanceData(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMeshInstance& meshInstance) const {
        RayQueryMeshInstanceData data{};
        data.vertexBufferDeviceAddress = vertexBufferAddress;
        data.indexBufferDeviceAddress = indexBufferAddress;
        data.mesh = meshInstance.mesh;
        data.material = meshInstance.material;
        return data;
    }

    VkAccelerationStructureInstanceKHR RayQueryScene::CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex) const {
        // instanceCustomIndex is the shader lookup into RayQueryBLASInstanceData
        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transform;
        instance.instanceCustomIndex = instanceCustomIndex;
        instance.mask = 0xff;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = accelerationStructureAddress;
        return instance;
    }
}
