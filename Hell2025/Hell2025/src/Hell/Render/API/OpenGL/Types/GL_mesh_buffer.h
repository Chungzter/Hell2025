#pragma once
#include "Hell/Render/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct OpenGLMeshBuffer {
    void Init(const VertexLayoutDescription& layout);
    void Reset();
    void InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset);
    void InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void InsertVertexWeights(const std::vector<VertexWeight>& vertexWeights, uint32_t insertOffset);
    void PreAllocate(size_t vertexCapacity, size_t indexCapacity, size_t vertexWeightCapacity);
    void ResizeVertexBuffer(size_t newCapacity, const std::vector<Vertex>& vertices);
    void ResizeIndexBuffer(size_t newCapacity, const std::vector<uint32_t>& indices);
    void ResizeVertexWeightBuffer(size_t newCapacity, const std::vector<VertexWeight>& vertexWeights);

    uint32_t GetVAO() const { return m_vao; }
    uint32_t GetVBO() const { return m_vbo; }
    uint32_t GetEBO() const { return m_ebo; }
    uint32_t GetVertexWeightSSBO() const { return m_vertexWeightSSBO; }

private:
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    uint32_t m_vertexWeightSSBO = 0;
    size_t m_vertexStride = 0;
};
