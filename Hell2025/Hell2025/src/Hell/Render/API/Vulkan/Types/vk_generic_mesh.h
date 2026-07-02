#pragma once

#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct VulkanGenericMesh {
    void UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout);
    void UpdateIndexData(const std::vector<uint32_t>& indices);
    void CleanUp();
    void Bind(VkCommandBuffer commandBuffer) const;

    size_t GetVertexCount() const { return m_vertexCount; }
    size_t GetIndexCount() const  { return m_indexCount; }
    VulkanBuffer* GetVertexBuffer() const;
    VulkanBuffer* GetIndexBuffer() const;

private:
    void ResizeVertexBuffer(size_t newCapacity);
    void ResizeIndexBuffer(size_t newCapacity);

    uint64_t m_vertexBufferId = 0;
    uint64_t m_indexBufferId = 0;
    size_t m_vertexStride = 0;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
};
