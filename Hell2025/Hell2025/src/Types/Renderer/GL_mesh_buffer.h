#pragma once

#include "Hell/Types.h"

#include <cstdint>
#include <vector>

struct OpenGLMeshBuffer {
    void Init(size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void Reset(size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset);
    void InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void PreAllocate(size_t maxVertices, size_t maxIndices, size_t& currentVertexCapacity, size_t& currentIndexCapacity);
    void ResizeVertexBuffer(size_t totalCount, std::vector<Vertex>& vertices, size_t& currentVertexCapacity);
    void ResizeIndexBuffer(size_t totalCount, std::vector<uint32_t>& indices, size_t& currentIndexCapacity);

    uint32_t GetVAO() const { return m_vao; }
    uint32_t GetVBO() const { return m_vbo; }
    uint32_t GetEBO() const { return m_ebo; }

private:
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
};