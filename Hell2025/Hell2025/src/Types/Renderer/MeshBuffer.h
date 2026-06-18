#pragma once
#include "Mesh.h"
#include <Hell/Types.h>

#include <cstdint>
#include <span>
#include <vector>
#include <unordered_map>

#include "GL_mesh_buffer.h" // TODO: move me to OpenGLResourceManager

struct MemoryBlock {
    size_t begin = 0;
    size_t end = 0;
    size_t GetSize() const { return end - begin; }
};

struct MeshBuffer {
    MeshBuffer() = default;
    MeshBuffer(const std::string& name);
    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) noexcept = default;
    MeshBuffer& operator=(MeshBuffer&&) noexcept = default;
    ~MeshBuffer() = default;

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
    const uint32_t GetVAO() const { return meshBufferGL.GetVAO(); }
    const uint32_t GetVBO() const { return meshBufferGL.GetVBO(); }
    const uint32_t GetEBO() const { return meshBufferGL.GetEBO(); }

private:
    void Initilize();
    int32_t AllocateExtraVertexSpace(size_t vertexCount);
    int32_t AllocateExtraIndexSpace(size_t indexCount);
    int32_t AddVertices(const std::vector<Vertex>& newVertices);
    int32_t AddIndices(const std::vector<uint32_t>& newIndices);
    
    std::string m_name = UNDEFINED_STRING;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<uint64_t, Mesh> m_meshes;
    std::vector<MemoryBlock> m_freeVertexMemoryBlocks;
    std::vector<MemoryBlock> m_freeIndexMemoryBlocks;
    uint64_t m_nextMeshId = 0;
    bool m_initilized = false;

    // OpenGL
    OpenGLMeshBuffer meshBufferGL;

    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
};
