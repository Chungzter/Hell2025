#include "GL_mesh_buffer.h"
#include <glad/gl.h>

void OpenGLMeshBuffer::Init(size_t& currentVertexCapacity, size_t& currentIndexCapacity) {
    if (m_vao != 0) {
        Reset(currentVertexCapacity, currentIndexCapacity);
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

void OpenGLMeshBuffer::Reset(size_t& currentVertexCapacity, size_t& currentIndexCapacity) {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;

    currentVertexCapacity = 0;
    currentIndexCapacity = 0;
}

void OpenGLMeshBuffer::InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset) {
    if (vertices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(Vertex);
    size_t byteSize = vertices.size() * sizeof(Vertex);

    glNamedBufferSubData(m_vbo, byteOffset, byteSize, vertices.data());
}

void OpenGLMeshBuffer::InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset) {
    if (indices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(uint32_t);
    size_t byteSize = indices.size() * sizeof(uint32_t);

    glNamedBufferSubData(m_ebo, byteOffset, byteSize, indices.data());
}

void OpenGLMeshBuffer::ResizeVertexBuffer(size_t totalCount, std::vector<Vertex>& vertices, size_t& currentVertexCapacity) {
    if (totalCount <= currentVertexCapacity) return;

    size_t newCapacity = 0;

    if (currentVertexCapacity == 0) {
        newCapacity = std::max(totalCount, (size_t)1024);
    }
    else {
        newCapacity = std::max(totalCount, currentVertexCapacity + (currentVertexCapacity / 2));
    }

    GLuint newVbo = 0;
    glCreateBuffers(1, &newVbo);
    glNamedBufferStorage(newVbo, newCapacity * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!vertices.empty()) {
        glNamedBufferSubData(newVbo, 0, vertices.size() * sizeof(Vertex), vertices.data());
    }

    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);

    m_vbo = newVbo;
    currentVertexCapacity = newCapacity;

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
}

void OpenGLMeshBuffer::ResizeIndexBuffer(size_t totalCount, std::vector<uint32_t>& indices, size_t& currentIndexCapacity) {
    if (totalCount <= currentIndexCapacity) return;

    size_t newCapacity = 0;

    if (currentIndexCapacity == 0) {
        newCapacity = std::max(totalCount, (size_t)1024);
    }
    else {
        newCapacity = std::max(totalCount, currentIndexCapacity + (currentIndexCapacity / 2));
    }

    GLuint newEbo = 0;
    glCreateBuffers(1, &newEbo);
    glNamedBufferStorage(newEbo, newCapacity * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!indices.empty()) {
        glNamedBufferSubData(newEbo, 0, indices.size() * sizeof(uint32_t), indices.data());
    }

    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_ebo = newEbo;
    currentIndexCapacity = newCapacity;

    glVertexArrayElementBuffer(m_vao, m_ebo);
}

void OpenGLMeshBuffer::PreAllocate(size_t maxVertices, size_t maxIndices, size_t& currentVertexCapacity, size_t& currentIndexCapacity) {
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);

    currentVertexCapacity = maxVertices;
    currentIndexCapacity = maxIndices;

    glNamedBufferStorage(m_vbo, maxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(m_ebo, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayElementBuffer(m_vao, m_ebo);
    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Vertex));
}