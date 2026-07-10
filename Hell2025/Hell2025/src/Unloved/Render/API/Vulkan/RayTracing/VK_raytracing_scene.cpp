#include "VK_raytracing_scene.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <array>

namespace VulkanRenderer {

    void RayTracingScene::Clear() {
        m_instances.clear();
        m_instanceData.clear();
        m_geometryData.clear();
        m_instanceBuffer = nullptr;
        m_instanceDataBuffer = nullptr;
        m_geometryDataBuffer = nullptr;
    }

    void RayTracingScene::Reserve(size_t instanceCount, size_t geometryDataCount) {
        m_instances.reserve(instanceCount);
        m_instanceData.reserve(instanceCount);
        m_geometryData.reserve(geometryDataCount);
    }

    void RayTracingScene::AddInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayTracingGeometryRange& range) {
        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_instanceData.size());

        RayTracingInstanceData& data = m_instanceData.emplace_back();
        data.geometryDataOffset = static_cast<uint32_t>(m_geometryData.size());
        data.geometryDataCount = 1;

        m_geometryData.push_back(CreateGeometryData(vertexBufferAddress, indexBufferAddress, range));
        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
    }

    void RayTracingScene::AddInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayTracingGeometryRange>& ranges) {
        if (ranges.empty()) return;

        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_instanceData.size());

        RayTracingInstanceData& data = m_instanceData.emplace_back();
        data.geometryDataOffset = static_cast<uint32_t>(m_geometryData.size());
        data.geometryDataCount = static_cast<uint32_t>(ranges.size());

        m_geometryData.reserve(m_geometryData.size() + ranges.size());
        for (const RayTracingGeometryRange& range : ranges) {
            m_geometryData.push_back(CreateGeometryData(vertexBufferAddress, indexBufferAddress, range));
        }

        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
    }

    bool RayTracingScene::HasInstances() const {
        return !m_instances.empty();
    }

    uint32_t RayTracingScene::GetInstanceCount() const {
        return static_cast<uint32_t>(m_instances.size());
    }

    VkDeviceSize RayTracingScene::GetTLASScratchSize(const VulkanFrameData& frameData) const {
        return static_cast<VkDeviceSize>(frameData.accelerationStructures.rayQueryTLASScratchSize);
    }

    bool RayTracingScene::Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData) {
        if (m_instances.empty()) return false;

        VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_instances.size();
        VkDeviceSize instanceDataSize = sizeof(RayTracingInstanceData) * m_instanceData.size();
        VkDeviceSize geometryDataSize = sizeof(RayTracingGeometryData) * m_geometryData.size();

        if (!EnsureBufferSize(frameData.buffers.rayQueryInstances, instanceBufferSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQueryInstanceData, instanceDataSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQueryGeometryData, geometryDataSize)) return false;

        m_instanceBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstances);
        m_instanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstanceData);
        m_geometryDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryGeometryData);
        if (!m_instanceBuffer) return false;
        if (!m_instanceDataBuffer) return false;
        if (!m_geometryDataBuffer) return false;

        m_instanceBuffer->UpdateData(m_instances.data(), instanceBufferSize);
        m_instanceDataBuffer->UpdateData(m_instanceData.data(), instanceDataSize);
        m_geometryDataBuffer->UpdateData(m_geometryData.data(), geometryDataSize);

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

        metadataUploadBarriers[0].buffer = m_instanceDataBuffer->GetBuffer();
        metadataUploadBarriers[0].size = instanceDataSize;
        metadataUploadBarriers[1].buffer = m_geometryDataBuffer->GetBuffer();
        metadataUploadBarriers[1].size = geometryDataSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(metadataUploadBarriers.size()), metadataUploadBarriers.data(), 0, nullptr);

        return true;
    }

    bool RayTracingScene::ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity) {
        if (!m_instanceBuffer) return false;
        if (frameData.accelerationStructures.rayQueryTLAS == 0) return false;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryTopLevelBuildSize(m_instanceBuffer->GetDeviceAddress(), instanceCapacity);
        if (!PrepareAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizeInfo)) return false;

        frameData.accelerationStructures.rayQueryTLASInstanceCapacity = instanceCapacity;
        frameData.accelerationStructures.rayQueryTLASScratchSize = sizeInfo.buildScratchSize;
        return true;
    }

    void RayTracingScene::RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress) {
        if (!m_instanceBuffer) return;
        RecordTopLevelBuild(commandBuffer, frameData.accelerationStructures.rayQueryTLAS, m_instanceBuffer->GetDeviceAddress(), GetInstanceCount(), scratchBaseAddress);
    }

    bool RayTracingScene::BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding) {
        if (!descriptorSet) return false;

        VulkanAccelerationStructure* tlas = VulkanResourceManager::GetAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS);
        if (!tlas || tlas->GetHandle() == VK_NULL_HANDLE) return false;

        descriptorSet->WriteAccelerationStructure(binding, tlas->GetHandle());
        descriptorSet->Update();
        return true;
    }

    RayTracingGeometryData RayTracingScene::CreateGeometryData(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayTracingGeometryRange& range) const {
        RayTracingGeometryData data{};
        data.vertexBufferDeviceAddress = vertexBufferAddress;
        data.indexBufferDeviceAddress = indexBufferAddress;
        data.baseVertex = range.baseVertex;
        data.baseIndex = range.baseIndex;
        data.vertexCount = range.vertexCount;
        data.indexCount = range.indexCount;
        data.blendingMode = range.blendingMode;
        data.materialIndex = range.materialIndex;
        data.shadowBit = range.shadowBit;
        return data;
    }

    VkAccelerationStructureInstanceKHR RayTracingScene::CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex) const {
        // instanceCustomIndex is the shader lookup into RayTracingInstanceData
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
