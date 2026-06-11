#include "GL_memory_tracker.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace OpenGLMemoryTracker {

    size_t g_framebufferAttachmentBytes = 0;
    size_t g_cubemapArrayBytes = 0;
    size_t g_vertexBufferBytes = 0;
    size_t g_ssboBufferBytes = 0;

    size_t GetBytesPerPixel(GLenum internalFormat) {
        switch (internalFormat) {
            case GL_R8: return 1;
            case GL_RG8: return 2;
            case GL_RGB8: return 3;
            case GL_RGBA8: return 4;
            case GL_R16F: return 2;
            case GL_RG16F: return 4;
            case GL_RGB16F: return 6;
            case GL_RGBA16F: return 8;
            case GL_R32F: return 4;
            case GL_RG32F: return 8;
            case GL_RGB32F: return 12;
            case GL_RGBA32F: return 16;
            case GL_DEPTH_COMPONENT16: return 2;
            case GL_DEPTH_COMPONENT24: return 3;
            case GL_DEPTH_COMPONENT32F: return 4;
            case GL_DEPTH24_STENCIL8: return 4;
            case GL_DEPTH32F_STENCIL8: return 5;
            default: return 0;
        }
    }

    void AddFramebufferAttachmentBytes(int width, int height, GLenum internalFormat, int samples) {
        g_framebufferAttachmentBytes += size_t(width) * size_t(height) * GetBytesPerPixel(internalFormat) * size_t(samples);
    }

    void RemoveFramebufferAttachmentBytes(int width, int height, GLenum internalFormat, int samples) {
        g_framebufferAttachmentBytes -= size_t(width) * size_t(height) * GetBytesPerPixel(internalFormat) * size_t(samples);
    }

    void AddCubemapArrayBytes(int resolution, int cubemapCount, GLenum internalFormat) {

    }

    void RemoveCubemapArrayBytes(int resolution, int cubemapCount, GLenum internalFormat) {

    }

    void AddVertexBufferBytes(size_t byteCount) {

    }

    void RemoveVertexBufferBytes(size_t byteCount) {

    }

    std::string PrettifySize(size_t byteCount) {
        constexpr size_t oneMegabyte = 1024 * 1024;

        if (byteCount <= oneMegabyte) {
            return std::to_string(byteCount) + " bytes";
        }

        float megabytes = static_cast<float>(byteCount) / static_cast<float>(oneMegabyte);

        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << megabytes << " MB";
        return stream.str();
    }


    size_t GetTotalBytes() {
        return g_cubemapArrayBytes +
            g_framebufferAttachmentBytes +
            g_ssboBufferBytes +
            g_vertexBufferBytes;
    }

    std::string GetReport() {
        std::string result;
        result += "Cubemap Arrays:  " + PrettifySize(g_cubemapArrayBytes) + "\n";
        result += "FBO Attachments: " + PrettifySize(g_framebufferAttachmentBytes) + "\n";
        result += "SSBOs:           " + PrettifySize(g_ssboBufferBytes) + "\n";
        result += "Vertex buffers:  " + PrettifySize(g_vertexBufferBytes) + "\n";
        result += "\n";
        result += "Total:           " + PrettifySize(GetTotalBytes()) + "\n";
        return result;
    }
}