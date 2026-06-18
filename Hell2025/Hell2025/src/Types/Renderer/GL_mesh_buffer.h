#pragma once

#include "Hell/Types.h"
#include "Hell/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

struct OpenGLMeshBuffer {
    void Init(const VertexLayoutDescription& layout, size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void Reset(size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void PreAllocate(size_t maxVertices, size_t maxIndices, size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void ResizeIndexBuffer(size_t totalCount, const std::vector<uint32_t>& indices, size_t& currentIndexCapacity);

    template<typename TVertex>
    void InsertVertices(const std::vector<TVertex>& vertices, uint32_t insertOffset) {
        InsertVertexData(vertices.data(), vertices.size(), insertOffset);
    }

    template<typename TVertex>
    void ResizeVertexBuffer(size_t totalCount, const std::vector<TVertex>& vertices, size_t& currentVertexCapacity) {
        ResizeVertexBufferData(totalCount, vertices.data(), vertices.size(), currentVertexCapacity);
    }

    uint32_t GetVAO() const { return m_vao; }
    uint32_t GetVBO() const { return m_vbo; }
    uint32_t GetEBO() const { return m_ebo; }

private:
    void InsertVertexData(const void* vertices, size_t vertexCount, uint32_t insertOffset);
    void ResizeVertexBufferData(size_t totalCount, const void* vertices, size_t vertexCount, size_t& currentVertexCapacity);

    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    size_t m_vertexStride = 0;
};
