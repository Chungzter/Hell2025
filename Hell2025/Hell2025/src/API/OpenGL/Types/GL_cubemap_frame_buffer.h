#pragma once
#include <glad/gl.h>
#include "../GL_Util.h"
#include <vector>
#include <iostream>
#include <string>
#include "GL_attachments.h"
#include <Hell/Types.h>

struct OpenGLCubemapFrameBuffer {
    OpenGLCubemapFrameBuffer() = default;
    OpenGLCubemapFrameBuffer(const std::string& name, int width);

    void Create(const std::string& name, int width);
    void CleanUp();
    void CreateAttachment(GLenum internalFormat, GLenum minFilter = GL_NEAREST, GLenum magFilter = GL_NEAREST);
    void CreateDepthAttachment(GLenum internalFormat);
    void Bind();
    void BindFaceByIndex(int32_t faceIndex);
    void SetViewport();
    void ClearFaceDepth(float depth);
    void ClearFaceColor(float r, float g, float b, float a);
    void ClearFaceColor(const glm::vec4& color);

    GLuint GetHandle() const      { return m_handle; }
    GLuint GetSize() const        { return m_size; }
    GLuint GetColorHandle() const { return m_colorAttachment.handle; }
    GLuint GetDepthHandle() const { return m_depthAttachment.handle; }

private:
    std::string m_name = "NULL";
    GLuint m_handle = 0;
    GLuint m_size = 0;

    ColorAttachment m_colorAttachment;
    DepthAttachment m_depthAttachment;

};