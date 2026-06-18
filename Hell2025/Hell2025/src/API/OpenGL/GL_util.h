#pragma once
#include <glad/gl.h>
#include "cmp_compressonatorlib/compressonator.h"

#include "Hell/TextureTypes.h"
#include "Hell/Types.h"

namespace OpenGLUtil {
    bool ExtensionExists(const std::string& extensionName);
    GLenum ImageFormatToGLFormat(ImageFormat format);
    GLenum ImageFormatToGLInternalFormat(ImageFormat format);
    GLenum ImageFormatToGLDataType(ImageFormat format);
    GLint GetFormatFromChannelCount(int channelCount);
    GLint GetInternalFormatFromChannelCount(int channelCount);
    uint32_t CMPFormatToGLFormat(CMP_FORMAT format);
    uint32_t CMPFormatToGLInternalFormat(CMP_FORMAT format);
    const char* GetGLSyncStatusString(GLenum result);
    const char* GLFormatToString(GLenum format);
    const char* GLInternalFormatToString(GLenum internalFormat); 
    const char* GLDataTypeToString(GLenum dataType);
    GLint GetChannelCountFromFormat(GLenum format);
    size_t CalculateCompressedDataSize(GLenum format, int width, int height);
    GLint TextureWrapModeToGLEnum(TextureWrapMode wrapMode);
    GLint TextureFilterToGLEnum(TextureFilter filter);
    GLenum GLInternalFormatToGLType(GLenum internalFormat);
    GLenum GLInternalFormatToGLFormat(GLenum internalFormat);
    GLint GetFormatFromInternalFormat(GLint internalFormat);
}
