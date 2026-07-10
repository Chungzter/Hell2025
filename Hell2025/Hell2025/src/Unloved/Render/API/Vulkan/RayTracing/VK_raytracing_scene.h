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
    struct RayTracingInstanceData {
        uint32_t geometryDataOffset = 0;
        uint32_t geometryDataCount = 0;
        uint32_t padding0 = 0;
        uint32_t padding1 = 0;
    };

    struct RayTracingGeometryData {
        uint64_t vertexBufferDeviceAddress = 0;
        uint64_t indexBufferDeviceAddress = 0;
        uint32_t baseVertex = 0;
        uint32_t baseIndex = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t blendingMode = 0;
        int32_t materialIndex = -1;
        uint32_t shadowBit = 0;
        uint32_t padding0 = 0;
    };

    struct RayTracingScene {
        void Clear();
        void Reserve(size_t instanceCount, size_t geometryDataCount);

        void AddInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayTracingGeometryRange& range);
        void AddInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayTracingGeometryRange>& ranges);

        bool HasInstances() const;
        uint32_t GetInstanceCount() const;
        VkDeviceSize GetTLASScratchSize(const VulkanFrameData& frameData) const;

        bool Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData);
        bool ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity);
        void RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress);
        bool BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding);

    private:
        RayTracingGeometryData CreateGeometryData(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayTracingGeometryRange& range) const;
        VkAccelerationStructureInstanceKHR CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex) const;

        std::vector<VkAccelerationStructureInstanceKHR> m_instances;
        std::vector<RayTracingInstanceData> m_instanceData;
        std::vector<RayTracingGeometryData> m_geometryData;

        VulkanBuffer* m_instanceBuffer = nullptr;
        VulkanBuffer* m_instanceDataBuffer = nullptr;
        VulkanBuffer* m_geometryDataBuffer = nullptr;
    };
}
