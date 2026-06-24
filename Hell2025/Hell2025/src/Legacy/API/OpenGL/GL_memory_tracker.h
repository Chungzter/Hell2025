#include <glad/gl.h>
#include <string>

namespace OpenGLMemoryTracker {

    void AddFramebufferAttachmentBytes(int width, int height, GLenum internalFormat, int samples = 1);
    void RemoveFramebufferAttachmentBytes(int width, int height, GLenum internalFormat, int samples = 1);

    void AddCubemapArrayBytes(int resolution, int cubemapCount, GLenum internalFormat, int mipLevels = 1);
    void RemoveCubemapArrayBytes(int resolution, int cubemapCount, GLenum internalFormat, int mipLevels = 1);

    void AddCubemapBytes(int resolution, GLenum internalFormat, int mipLevels = 1);
    void RemoveCubemapBytes(int resolution, GLenum internalFormat, int mipLevels = 1);

    void AddVertexBufferBytes(size_t byteCount);
    void RemoveVertexBufferBytes(size_t byteCount);

    void AddSSBOBytes(size_t byteCount);
    void RemoveSSBOBytes(size_t byteCount);

    size_t GetTotalBytes();
    std::string GetReportNames();
    std::string GetReportBytes();
}