#include "GL_mesh2D.h"
#include <glad/gl.h>

void OpenGLMesh2D::UpdateVertexBuffer(std::vector<Vertex2D>& vertices, std::vector<uint32_t>& indices) {
    m_indexCount = (GLuint)indices.size();

    if (m_indexCount == 0) {
        return;
    }

    GLsizei vertexBufferSize = (GLsizei)(vertices.size() * sizeof(Vertex2D));
    GLsizei indexBufferSize = (GLsizei)(indices.size() * sizeof(uint32_t));

    if (vertexBufferSize > m_vertexBufferSize || indexBufferSize > m_indexBufferSize) {
        CleanUp();
        Create();

        m_vertexBufferSize = vertexBufferSize;
        m_indexBufferSize = indexBufferSize;

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, nullptr, GL_DYNAMIC_DRAW);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBufferSize, vertices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufferSize, indices.data());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, color));
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLMesh2D::Create() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

void OpenGLMesh2D::CleanUp() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        m_vao = 0;
        m_vbo = 0;
        m_ebo = 0;
    }
}