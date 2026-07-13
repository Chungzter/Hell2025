#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"

#include "Unloved/Render/API/Vulkan/VK_frame_data.h"
#include "Unloved/Render/RendererTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    struct RayQueryBLASInstanceData {
        uint32_t meshInstanceDataOffset = 0;
        uint32_t meshInstanceDataCount = 0;
        uint32_t padding0 = 0;
        uint32_t padding1 = 0;
    };

    struct RayQueryMeshInstanceData {
        uint64_t vertexBufferDeviceAddress = 0;
        uint64_t indexBufferDeviceAddress = 0;
        RayQueryMesh mesh;
        RayQueryMaterial material;
    };

    struct RayQueryScene {
        void Clear();
        void Reserve(size_t blasInstanceCount, size_t meshInstanceDataCount);

        void AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMeshInstance& meshInstance);
        void AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances);

        bool HasInstances() const;
        uint32_t GetInstanceCount() const;
        VkDeviceSize GetTLASScratchSize(const VulkanFrameData& frameData) const;

        bool Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData);
        bool ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity);
        void RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress);
        bool BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding);

    private:
        RayQueryMeshInstanceData CreateMeshInstanceData(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMeshInstance& meshInstance) const;
        VkAccelerationStructureInstanceKHR CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex) const;

        std::vector<VkAccelerationStructureInstanceKHR> m_instances;
        std::vector<RayQueryBLASInstanceData> m_blasInstanceData;
        std::vector<RayQueryMeshInstanceData> m_meshInstanceData;

        VulkanBuffer* m_instanceBuffer = nullptr;
        VulkanBuffer* m_blasInstanceDataBuffer = nullptr;
        VulkanBuffer* m_meshInstanceDataBuffer = nullptr;
    };
}
