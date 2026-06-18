#pragma once

#include "API/OpenGL/Types/GL_mesh_buffer.h" // TODO: move me to OpenGLResourceManager
#include "Types/Renderer/Mesh.h"

#include "Hell/Types.h"
#include "Hell/VertexAttributes.h"

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

struct MemoryBlock {
    size_t begin = 0;
    size_t end = 0;
    size_t GetSize() const { return end - begin; }
};

template<typename TVertex>
struct MeshBufferT {
    MeshBufferT() = default;
    MeshBufferT(const std::string& name);
    MeshBufferT(const MeshBufferT&) = delete;
    MeshBufferT& operator=(const MeshBufferT&) = delete;
    MeshBufferT(MeshBufferT&&) noexcept = default;
    MeshBufferT& operator=(MeshBufferT&&) noexcept = default;
    ~MeshBufferT() = default;

    void PreAllocate(size_t maxVertices, size_t maxIndices);
    void RemoveMesh(uint64_t meshIndex);
    void Reset();
    void PrintDebugInfo();

    uint64_t AddMesh(const std::vector<TVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = UNDEFINED_STRING);

    Mesh* GetMeshById(uint64_t meshId);
    std::span<TVertex> GetMeshVertexSpan(uint64_t meshId);
    std::span<uint32_t> GetMeshIndexSpan(uint64_t meshId);

    size_t GetMeshCount()                 { return m_meshes.size(); }
    size_t GetAllocatedVertexCount()      { return m_vertices.size(); }
    size_t GetAllocatedIndexCount()       { return m_indices.size(); }
    std::vector<TVertex>& GetVertices()   { return m_vertices; }
    std::vector<uint32_t>& GetIndices()   { return m_indices; }

    // OpenGL
    uint32_t GetVAO() const { return m_meshBufferGL.GetVAO(); }
    uint32_t GetVBO() const { return m_meshBufferGL.GetVBO(); }
    uint32_t GetEBO() const { return m_meshBufferGL.GetEBO(); }

private:
    void Initialize();
    int32_t AllocateExtraVertexSpace(size_t vertexCount);
    int32_t AllocateExtraIndexSpace(size_t indexCount);
    int32_t AddVertices(const std::vector<TVertex>& newVertices);
    int32_t AddIndices(const std::vector<uint32_t>& newIndices);

    std::string m_name = UNDEFINED_STRING;
    std::vector<TVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<uint64_t, Mesh> m_meshes;
    std::vector<MemoryBlock> m_freeVertexMemoryBlocks;
    std::vector<MemoryBlock> m_freeIndexMemoryBlocks;
    uint64_t m_nextMeshId = 0;
    bool m_initialized = false;

    // OpenGL
    OpenGLMeshBuffer m_meshBufferGL;

    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
};

using MeshBuffer = MeshBufferT<Vertex>;

#include "MeshBuffer.inl"
