#pragma once

#include "Hell/Types.h"
#include "Hell/VertexAttributes.h"

#include <glad/gl.h>

#include <cstdint>
#include <vector>

struct OpenGLMesh2D {
    void Create();
    void CleanUp();
    void UpdateVertexBuffer(std::vector<Vertex2D>& vertices, std::vector<uint32_t>& indices);

    GLuint GetVAO() const         { return m_vao; }
    GLsizei GetIndexCount() const { return m_indexCount; }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_indexCount = 0;
    GLsizei m_vertexBufferSize = 0;
    GLsizei m_indexBufferSize = 0;
};