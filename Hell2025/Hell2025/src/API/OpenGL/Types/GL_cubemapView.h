#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>

struct OpenGLCubemapView {
    OpenGLCubemapView() = default;
    OpenGLCubemapView(const std::vector<GLuint>& tex2D);
    void CreateCubemap(const std::vector<GLuint>& tex2D);
    GLuint GetHandle() const;

private:
    GLuint m_handle = 0;
};
