#include "GL_shadow_cube_map_array.h"
#include "API/OpenGL/GL_backEnd.h"
#include "API/OpenGL/GL_memory_tracker.h"
#include <glad/gl.h>
#include <iostream>

void OpenGLShadowCubeMapArray::Init(unsigned int numberOfCubemaps, int size) {
    m_numberOfCubemaps = numberOfCubemaps;
    m_size = size;
    m_internalFormat = GL_DEPTH_COMPONENT16;
    m_mipLevels = 1;

    glGenFramebuffers(1, &m_handle);
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depthTexture);

    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, m_mipLevels, m_internalFormat, size, size, 6 * m_numberOfCubemaps);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "OpenGLShadowCubeMapArray::Init(): Framebuffer not complete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &m_textureView);
    glTextureView( m_textureView, GL_TEXTURE_2D_ARRAY, m_depthTexture, m_internalFormat, 0, m_mipLevels, 0, 6 * m_numberOfCubemaps);

    OpenGLMemoryTracker::AddCubemapArrayBytes(m_size, m_numberOfCubemaps, m_internalFormat, m_mipLevels);
}

void OpenGLShadowCubeMapArray::CleanUp() {
    glDeleteTextures(1, &m_depthTexture);
    glDeleteTextures(1, &m_textureView);
    glDeleteFramebuffers(1, &m_handle);
    OpenGLMemoryTracker::RemoveCubemapArrayBytes(m_size, m_numberOfCubemaps, m_internalFormat, m_mipLevels);
}

void OpenGLShadowCubeMapArray::ClearDepthLayer(int layerIndex, float clearValue) {
    OpenGLBackEnd::SetDepthClearValue(clearValue);

    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glViewport(0, 0, m_size, m_size);

    for (int face = 0; face < 6; ++face) {
        int layer = layerIndex * 6 + face;
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0, layer);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void OpenGLShadowCubeMapArray::ClearDepthLayers(float clearValue) {
    glClearTexImage(m_depthTexture, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValue);
}
