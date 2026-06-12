#include "GL_shadow_map_array.h"

#include "API/OpenGL/GL_memory_tracker.h"
#include <iostream>

void OpenGLShadowMapArray::Init(unsigned int layerCount, int size, int internalFormat) {
    m_layerCount = layerCount;
    m_size = size;
    m_internalFormat = internalFormat;
    m_mipLevels = 1;

    glCreateFramebuffers(1, &m_handle);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_depthTexture);
    glTextureStorage3D(m_depthTexture, 1, internalFormat, size, size, layerCount);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    glNamedFramebufferDrawBuffer(m_handle, GL_NONE);
    glNamedFramebufferReadBuffer(m_handle, GL_NONE);

    if (glCheckNamedFramebufferStatus(m_handle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "OpenGLShadowMapArray::Init() failed: Framebuffer not complete!\n";
    }

    OpenGLMemoryTracker::AddCubemapBytes(m_size, m_internalFormat, m_mipLevels);
}

void OpenGLShadowMapArray::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void OpenGLShadowMapArray::SetTextureLayer(unsigned int layerIndex) {
    if (layerIndex >= m_layerCount) return;
    glNamedFramebufferTextureLayer(m_handle, GL_DEPTH_ATTACHMENT, m_depthTexture, 0, layerIndex);
}

void OpenGLShadowMapArray::SetViewport() {
    glViewport(0, 0, m_size, m_size);
}

void OpenGLShadowMapArray::CleanUp() {
    glDeleteTextures(1, &m_depthTexture);
    glDeleteFramebuffers(1, &m_handle);
    m_depthTexture = 0;
    m_handle = 0;
    m_layerCount = 0;
    OpenGLMemoryTracker::RemoveCubemapBytes(m_size, m_internalFormat, m_mipLevels);
}

void OpenGLShadowMapArray::ClearDepth() {
    const float clearDepthValue = 1.0f;
    glClearNamedFramebufferfv(m_handle, GL_DEPTH, 0, &clearDepthValue);
}