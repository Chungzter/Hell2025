#include "MeshBuffer.h"
#include <algorithm>
#include <Hell/Logging.h>
#include <Util/Util.h>

void MeshBuffer::Initilize() {
    Reset();
    InitOpenGL();
    m_initilized = true;
}

void MeshBuffer::Reset() {
    m_meshes.clear();
    m_vertices.clear();
    m_indices.clear();

    m_freeVertexMemoryBlocks.clear();
    m_freeIndexMemoryBlocks.clear();

    m_nextMeshId = 0;

    ResetOpenGL();

    m_initilized = false;
}

uint64_t MeshBuffer::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name) {
    if (!m_initilized) Initilize();

    if (vertices.empty() || indices.empty()) return 0;

    m_nextMeshId++;

    Mesh& mesh = m_meshes[m_nextMeshId];
    mesh.baseVertex = AddVertices(vertices);
    mesh.baseIndex = AddIndices(indices);
    mesh.vertexCount = vertices.size();
    mesh.indexCount = indices.size();
    mesh.name = name;

    // Compute axis aligned bounding box limits
    glm::vec3 aabbMin(std::numeric_limits<float>::max());
    glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

    for (const Vertex& v : vertices) {
        aabbMin = glm::min(aabbMin, v.position);
        aabbMax = glm::max(aabbMax, v.position);
    }

    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.extents = (aabbMax - aabbMin) * 0.5f;

    return m_nextMeshId;
}

int32_t MeshBuffer::AddVertices(const std::vector<Vertex>& newVertices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free vertex block
    for (int32_t i = 0; i < (int32_t)m_freeVertexMemoryBlocks.size(); i++) {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[i];

        if (newVertices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_vertices.size());
        size_t requiredCount = m_vertices.size() + newVertices.size();

        // Grow capacity by 1.5x
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_vertices.size() * 1.5));

        ResizeOpenGLVertexBuffer(newCount);

        m_vertices.resize(newCount);
        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);

        InsertVerticesToOpenGLVertexBuffer(newVertices, insertOffset);

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeVertexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = memoryBlock.begin;

        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);
        InsertVerticesToOpenGLVertexBuffer(newVertices, insertOffset);

        // Handle block resizing
        if (memoryBlock.GetSize() == newVertices.size()) {
            m_freeVertexMemoryBlocks.erase(m_freeVertexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newVertices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AddIndices(const std::vector<uint32_t>& newIndices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free index block
    for (int32_t i = 0; i < (int32_t)m_freeIndexMemoryBlocks.size(); i++) {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[i];

        if (newIndices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_indices.size());
        size_t requiredCount = m_indices.size() + newIndices.size();

        // Grow capacity by 1.5x
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_indices.size() * 1.5));

        ResizeOpenGLIndexBuffer(newCount);

        m_indices.resize(newCount);
        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);

        InsertIndicesToOpenGLIndexBuffer(newIndices, insertOffset);

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeIndexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = memoryBlock.begin;

        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);
        InsertIndicesToOpenGLIndexBuffer(newIndices, insertOffset);

        // Handle block resizing
        if (memoryBlock.GetSize() == newIndices.size()) {
            m_freeIndexMemoryBlocks.erase(m_freeIndexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newIndices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AllocateExtraVertexSpace(size_t vertexCount) {
    size_t blockBegin = m_vertices.size();
    size_t blockEnd = m_vertices.size() + vertexCount;

    ResizeOpenGLVertexBuffer(blockEnd);

    MemoryBlock& block = m_freeVertexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_vertices.resize(blockEnd);

    return (int32_t)(m_freeVertexMemoryBlocks.size() - 1);
}

int32_t MeshBuffer::AllocateExtraIndexSpace(size_t indexCount) {
    size_t blockBegin = m_indices.size();
    size_t blockEnd = m_indices.size() + indexCount;

    ResizeOpenGLIndexBuffer(blockEnd);

    MemoryBlock& block = m_freeIndexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_indices.resize(blockEnd);

    return (int32_t)(m_freeIndexMemoryBlocks.size() - 1);
}

void MeshBuffer::RemoveMesh(uint64_t meshIndex) {
    auto it = m_meshes.find(meshIndex);
    if (it == m_meshes.end()) return;

    Mesh& mesh = it->second;

    // Create free vertex memory block
    MemoryBlock vertexBlock;
    vertexBlock.begin = mesh.baseVertex;
    vertexBlock.end = mesh.baseVertex + mesh.vertexCount;
    m_freeVertexMemoryBlocks.push_back(vertexBlock);

    // Sort vertex blocks by starting offset
    std::sort(m_freeVertexMemoryBlocks.begin(), m_freeVertexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
    });

    // Merge adjacent free vertex memory blocks
    std::vector<MemoryBlock> mergedVertexBlocks;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        if (mergedVertexBlocks.empty()) {
            mergedVertexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedVertexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedVertexBlocks.push_back(block);
            }
        }
    }
    m_freeVertexMemoryBlocks = std::move(mergedVertexBlocks);

    // Create free index memory block
    MemoryBlock indexBlock;
    indexBlock.begin = mesh.baseIndex;
    indexBlock.end = mesh.baseIndex + mesh.indexCount;
    m_freeIndexMemoryBlocks.push_back(indexBlock);

    // Sort index blocks by starting offset
    std::sort(m_freeIndexMemoryBlocks.begin(), m_freeIndexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
        });

    // Merge adjacent free vertex memory blocks
    std::vector<MemoryBlock> mergedIndexBlocks;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        if (mergedIndexBlocks.empty()) {
            mergedIndexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedIndexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedIndexBlocks.push_back(block);
            }
        }
    }
    m_freeIndexMemoryBlocks = std::move(mergedIndexBlocks);

    // Remove the mesh
    m_meshes.erase(it);
}

void MeshBuffer::PreAllocate(size_t maxVertices, size_t maxIndices) {
    if (maxVertices == 0 || maxIndices == 0) {
        Logging::Warning() << "MeshBufferV2::PreAllocate() called with zero " << maxVertices << " vertices and " << maxIndices << " indices\n";
        return;
    }

    Reset();
    InitOpenGL();

    m_vertices.resize(maxVertices);
    m_indices.resize(maxIndices);

    // Allocate new GPU memory
    PreAllocateOpenGL(maxVertices, maxIndices);

    // Add one continuous free vertex memory block
    MemoryBlock initialVertexBlock;
    initialVertexBlock.begin = 0;
    initialVertexBlock.end = static_cast<uint32_t>(maxVertices);
    m_freeVertexMemoryBlocks.push_back(initialVertexBlock);

    // Add one continuous free index memory block
    MemoryBlock initialIndexBlock;
    initialIndexBlock.begin = 0;
    initialIndexBlock.end = static_cast<uint32_t>(maxIndices);
    m_freeIndexMemoryBlocks.push_back(initialIndexBlock);

    m_initilized = true;
}

Mesh* MeshBuffer::GetMeshById(uint64_t meshId) {
    auto it = m_meshes.find(meshId);
    if (it != m_meshes.end()) {
        return &it->second;
    }

    return nullptr;
}

std::span<Vertex> MeshBuffer::GetMeshVertexSpan(uint64_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<Vertex>(m_vertices.data() + mesh->baseVertex, mesh->vertexCount);
}

std::span<uint32_t> MeshBuffer::GetMeshIndexSpan(uint64_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<uint32_t>(m_indices.data() + mesh->baseIndex, mesh->indexCount);
}

void MeshBuffer::PrintDebugInfo() {
    size_t usedVertexCount = 0;
    size_t usedIndexCount = 0;

    for (const auto& [meshId, mesh] : m_meshes) {
        usedVertexCount += mesh.vertexCount;
        usedIndexCount += mesh.indexCount;
    }

    size_t freeVertexCount = 0;
    size_t largestFreeVertexBlock = 0;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeVertexCount += blockSize;
        largestFreeVertexBlock = std::max(largestFreeVertexBlock, blockSize);
    }

    size_t freeIndexCount = 0;
    size_t largestFreeIndexBlock = 0;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeIndexCount += blockSize;
        largestFreeIndexBlock = std::max(largestFreeIndexBlock, blockSize);
    }

    std::string message;
    message += "MeshBufferV2 Debug Info\n";
    message += "\n";

    message += "Meshes\n";
    message += "  Mesh count: " + std::to_string(GetMeshCount()) + "\n";
    message += "  Used vertex count: " + std::to_string(usedVertexCount) + "\n";
    message += "  Used index count: " + std::to_string(usedIndexCount) + "\n";
    message += "\n";

    message += "CPU storage\n";
    message += "  CPU allocated vertex count: " + std::to_string(GetAllocatedVertexCount()) + "\n";
    message += "  CPU allocated index count: " + std::to_string(GetAllocatedIndexCount()) + "\n";
    message += "  CPU free vertex count: " + std::to_string(freeVertexCount) + "\n";
    message += "  CPU free index count: " + std::to_string(freeIndexCount) + "\n";
    message += "\n";

    message += "GPU storage\n";
    message += "  GPU vertex capacity: " + std::to_string(m_vertexGpuCapacity) + "\n";
    message += "  GPU index capacity: " + std::to_string(m_indexGpuCapacity) + "\n";
    message += "\n";

    message += "Fragmentation\n";
    message += "  Free vertex block count: " + std::to_string(m_freeVertexMemoryBlocks.size()) + "\n";
    message += "  Free index block count:  " + std::to_string(m_freeIndexMemoryBlocks.size()) + "\n";
    message += "  Largest free vertex block: " + std::to_string(largestFreeVertexBlock) + "\n";
    message += "  Largest free index block:  " + std::to_string(largestFreeIndexBlock) + "\n";
    message += "\n";

    message += "Free vertex blocks\n";
    for (size_t i = 0; i < m_freeVertexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeVertexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Free index blocks\n";
    for (size_t i = 0; i < m_freeIndexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeIndexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Mesh list\n";
    for (const auto& [meshId, mesh] : m_meshes) {
        message += "  Mesh id: " + std::to_string(meshId) + "\n";
        message += "    Name: " + mesh.name + "\n";
        message += "    Base vertex: " + std::to_string(mesh.baseVertex) + "\n";
        message += "    Vertex count: " + std::to_string(mesh.vertexCount) + "\n";
        message += "    Base index: " + std::to_string(mesh.baseIndex) + "\n";
        message += "    Index count: " + std::to_string(mesh.indexCount) + "\n";
        message += "    AABB min: " + Util::Vec3ToString(mesh.aabbMin) + "\n";
        message += "    AABB max: " + Util::Vec3ToString(mesh.aabbMax) + "\n";
        message += "    Extents: " + Util::Vec3ToString(mesh.extents) + "\n";
        message += "\n";
    }

    Logging::Debug() << message << "\n";
}

// OpenGL
#include <glad/gl.h>

void MeshBuffer::InitOpenGL() {
    if (m_vao != 0) {
        ResetOpenGL();
    }

    glCreateVertexArrays(1, &m_vao);

    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(m_vao, 0, 0);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(m_vao, 1, 0);

    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribBinding(m_vao, 2, 0);

    glEnableVertexArrayAttrib(m_vao, 3);
    glVertexArrayAttribFormat(m_vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
    glVertexArrayAttribBinding(m_vao, 3, 0);
}

void MeshBuffer::ResetOpenGL() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;

    m_vertexGpuCapacity = 0;
    m_indexGpuCapacity = 0;
}

void MeshBuffer::InsertVerticesToOpenGLVertexBuffer(const std::vector<Vertex>& vertices, uint32_t insertOffset) {
    if (vertices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(Vertex);
    size_t byteSize = vertices.size() * sizeof(Vertex);

    glNamedBufferSubData(m_vbo, byteOffset, byteSize, vertices.data());
}

void MeshBuffer::InsertIndicesToOpenGLIndexBuffer(const std::vector<uint32_t>& indices, uint32_t insertOffset) {
    if (indices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(uint32_t);
    size_t byteSize = indices.size() * sizeof(uint32_t);

    glNamedBufferSubData(m_ebo, byteOffset, byteSize, indices.data());
}

void MeshBuffer::ResizeOpenGLVertexBuffer(size_t totalCount) {
    if (totalCount <= m_vertexGpuCapacity) return;

    size_t newGpuCapacity = 0;

    if (m_vertexGpuCapacity == 0) {
        newGpuCapacity = std::max(totalCount, (size_t)1024);
    }
    else {
        newGpuCapacity = std::max(totalCount, m_vertexGpuCapacity + (m_vertexGpuCapacity / 2));
    }

    GLuint newVbo = 0;
    glCreateBuffers(1, &newVbo);
    glNamedBufferStorage(newVbo, newGpuCapacity * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!m_vertices.empty()) {
        glNamedBufferSubData(newVbo, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
    }

    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);

    m_vbo = newVbo;
    m_vertexGpuCapacity = newGpuCapacity;

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
}

void MeshBuffer::ResizeOpenGLIndexBuffer(size_t totalCount) {
    if (totalCount <= m_indexGpuCapacity) return;

    size_t newGpuCapacity = 0;

    if (m_indexGpuCapacity == 0) {
        newGpuCapacity = std::max(totalCount, (size_t)1024);
    }
    else {
        newGpuCapacity = std::max(totalCount, m_indexGpuCapacity + (m_indexGpuCapacity / 2));
    }

    GLuint newEbo = 0;
    glCreateBuffers(1, &newEbo);
    glNamedBufferStorage(newEbo, newGpuCapacity * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!m_indices.empty()) {
        glNamedBufferSubData(newEbo, 0, m_indices.size() * sizeof(uint32_t), m_indices.data());
    }

    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_ebo = newEbo;
    m_indexGpuCapacity = newGpuCapacity;

    glVertexArrayElementBuffer(m_vao, m_ebo);
}

void MeshBuffer::PreAllocateOpenGL(size_t maxVertices, size_t maxIndices) {
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);

    m_vertexGpuCapacity = maxVertices;
    m_indexGpuCapacity = maxIndices;

    glNamedBufferStorage(m_vbo, maxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(m_ebo, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayElementBuffer(m_vao, m_ebo);
    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
}