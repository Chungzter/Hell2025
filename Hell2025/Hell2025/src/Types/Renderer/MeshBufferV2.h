#pragma once
#include "Mesh.h"
#include <Hell/Types.h>

#include <span>
#include <vector>
#include <unordered_map>

struct MemoryBlock {
    size_t begin = 0;
    size_t end = 0;
    size_t GetSize() const { return end - begin; }
};

struct MeshBufferV2 {
    MeshBufferV2() = default;
    ~MeshBufferV2() = default;
    MeshBufferV2(const MeshBufferV2&) = delete;
    MeshBufferV2& operator=(const MeshBufferV2&) = delete;
    MeshBufferV2(MeshBufferV2&&) = delete;
    MeshBufferV2& operator=(MeshBufferV2&&) = delete;

    void PreAllocate(size_t maxVertices, size_t maxIndices);
    void RemoveMesh(uint64_t meshIndex);
    void Reset();
    void PrintDebugInfo();

    uint64_t AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = UNDEFINED_STRING);

    Mesh* GetMeshById(uint64_t meshId);
    std::span<Vertex> GetMeshVertexSpan(uint64_t meshId);
    std::span<uint32_t> GetMeshIndexSpan(uint64_t meshId);

    size_t GetMeshCount()               { return m_meshes.size(); }
    size_t GetAllocatedVertexCount()    { return m_vertices.size(); }
    size_t GetAllocatedIndexCount()     { return m_indices.size(); }
    std::vector<Vertex>& GetVertices()  { return m_vertices; }
    std::vector<uint32_t>& GetIndices() { return m_indices; }

    // OpenGL
    const uint32_t GetVAO() const { return m_vao; }
    const uint32_t GetVBO() const { return m_vbo; }
    const uint32_t GetEBO() const { return m_ebo; }

private:
    void Initilize();
    int32_t AllocateExtraVertexSpace(size_t vertexCount);
    int32_t AllocateExtraIndexSpace(size_t indexCount);
    int32_t AddVertices(const std::vector<Vertex>& newVertices);
    int32_t AddIndices(const std::vector<uint32_t>& newIndices);

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<uint64_t, Mesh> m_meshes;
    std::vector<MemoryBlock> m_freeVertexMemoryBlocks;
    std::vector<MemoryBlock> m_freeIndexMemoryBlocks;
    uint64_t m_nextMeshId = 0;
    bool m_initilized = false;

    // OpenGL
    void InitOpenGL();
    void ResetOpenGL();
    void InsertVerticesToOpenGLVertexBuffer(const std::vector<Vertex>& vertices, uint32_t insertOffset);
    void InsertIndicesToOpenGLIndexBuffer(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void PreAllocateOpenGL(size_t maxVertices, size_t maxIndices);
    void ResizeOpenGLVertexBuffer(size_t totalCount);
    void ResizeOpenGLIndexBuffer(size_t totalCount);

    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    size_t m_vertexGpuCapacity = 0;
    size_t m_indexGpuCapacity = 0;
};