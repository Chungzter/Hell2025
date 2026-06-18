#include "GL_mesh_buffer.h"

#include <glad/gl.h>

#include <algorithm>

namespace {
    GLenum GetOpenGLType(VertexAttributeType type) {
        switch (type) {
            case VertexAttributeType::Float:       return GL_FLOAT;
            case VertexAttributeType::Int:         return GL_INT;
            case VertexAttributeType::UnsignedInt: return GL_UNSIGNED_INT;
        }

        return GL_FLOAT;
    }

    bool IsIntegerAttribute(VertexAttributeType type) {
        return type == VertexAttributeType::Int || type == VertexAttributeType::UnsignedInt;
    }
}

void OpenGLMeshBuffer::Init(const VertexLayoutDescription& layout, size_t& currentVertexCapacity, size_t& currentIndexCapacity) {
    if (m_vao != 0) {
        Reset(currentVertexCapacity, currentIndexCapacity);
    }

    m_vertexStride = layout.stride;
    glCreateVertexArrays(1, &m_vao);

    for (const VertexAttribute& attribute : layout.attributes) {
        glEnableVertexArrayAttrib(m_vao, attribute.location);

        if (IsIntegerAttribute(attribute.type)) {
            glVertexArrayAttribIFormat(m_vao, attribute.location, attribute.componentCount, GetOpenGLType(attribute.type), static_cast<GLuint>(attribute.offset) );
        }
        else {
            glVertexArrayAttribFormat(m_vao, attribute.location, attribute.componentCount, GetOpenGLType(attribute.type), attribute.normalized ? GL_TRUE : GL_FALSE, static_cast<GLuint>(attribute.offset));
        }

        glVertexArrayAttribBinding(m_vao, attribute.location, 0);
    }
}

void OpenGLMeshBuffer::Reset(size_t& currentVertexCapacity, size_t& currentIndexCapacity) {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_vertexStride = 0;

    currentVertexCapacity = 0;
    currentIndexCapacity = 0;
}

void OpenGLMeshBuffer::InsertVertexData(const void* vertices, size_t vertexCount, uint32_t insertOffset) {
    if (vertices == nullptr || vertexCount == 0) return;

    size_t byteOffset = insertOffset * m_vertexStride;
    size_t byteSize = vertexCount * m_vertexStride;

    glNamedBufferSubData(m_vbo, byteOffset, byteSize, vertices);
}

void OpenGLMeshBuffer::InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset) {
    if (indices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(uint32_t);
    size_t byteSize = indices.size() * sizeof(uint32_t);

    glNamedBufferSubData(m_ebo, byteOffset, byteSize, indices.data());
}

void OpenGLMeshBuffer::ResizeVertexBufferData(size_t totalCount, const void* vertices, size_t vertexCount, size_t& currentVertexCapacity) {
    if (totalCount <= currentVertexCapacity) return;

    size_t newCapacity = 0;

    if (currentVertexCapacity == 0) {
        newCapacity = std::max(totalCount, static_cast<size_t>(1024));
    }
    else {
        newCapacity = std::max(totalCount, currentVertexCapacity + (currentVertexCapacity / 2));
    }

    GLuint newVbo = 0;
    glCreateBuffers(1, &newVbo);
    glNamedBufferStorage(newVbo, newCapacity * m_vertexStride, nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (vertices != nullptr && vertexCount > 0) {
        glNamedBufferSubData(newVbo, 0, vertexCount * m_vertexStride, vertices);
    }

    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);

    m_vbo = newVbo;
    currentVertexCapacity = newCapacity;

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, static_cast<GLsizei>(m_vertexStride));
}

void OpenGLMeshBuffer::ResizeIndexBuffer(size_t totalCount, const std::vector<uint32_t>& indices, size_t& currentIndexCapacity) {
    if (totalCount <= currentIndexCapacity) return;

    size_t newCapacity = 0;

    if (currentIndexCapacity == 0) {
        newCapacity = std::max(totalCount, static_cast<size_t>(1024));
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

    glNamedBufferStorage(m_vbo, maxVertices * m_vertexStride, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(m_ebo, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayElementBuffer(m_vao, m_ebo);
    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, static_cast<GLsizei>(m_vertexStride));
}
