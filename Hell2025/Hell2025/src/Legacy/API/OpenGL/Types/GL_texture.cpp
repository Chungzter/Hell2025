#include "GL_texture.h"
#include "Hell/Logging.h"

#include "API/OpenGL/GL_util.h"
#include "Hell/Backend/BackEnd.h"
#include "Util/Util.h"

#include <algorithm>
#include <iostream>

namespace {
    constexpr GLenum GL_COMPRESSED_SRGB_S3TC_DXT1 = 0x8C4C;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1 = 0x8C4D;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3 = 0x8C4E;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5 = 0x8C4F;

    size_t GetBytesPerPixel(GLint internalFormat) {
        switch (internalFormat) {
            case GL_R8: return 1;
            case GL_RG8: return 2;
            case GL_RGB8:
            case GL_SRGB8: return 3;
            case GL_RGBA8:
            case GL_SRGB8_ALPHA8: return 4;
            case GL_R16:
            case GL_R16F: return 2;
            case GL_RG16F: return 4;
            case GL_RGB16F: return 6;
            case GL_RGBA16F: return 8;
            case GL_R11F_G11F_B10F: return 4;
            case GL_R32F: return 4;
            case GL_RG32F: return 8;
            case GL_RGB32F: return 12;
            case GL_RGBA32F: return 16;
            default: return 0;
        }
    }

    size_t GetCompressedBlockSize(GLint internalFormat) {
        switch (internalFormat) {
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_S3TC_DXT1:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
            case GL_COMPRESSED_RED_RGTC1:
                return 8;
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
            case GL_COMPRESSED_RGBA_BPTC_UNORM:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                return 16;
            default:
                return 0;
        }
    }
}

GLuint64 OpenGLTexture::GetBindlessID() {
    return m_bindlessID;
}

void OpenGLTexture::Create(int width, int height, int internalFormat, int mipmapLevelCount) {
    if (m_handle != 0 || m_bindlessID != 0) {
        Reset();
    }

    m_width = width;
    m_height = height;
    m_mipmapLevelCount = mipmapLevelCount;
    m_internalFormat = internalFormat;
    m_format = OpenGLUtil::GetFormatFromInternalFormat(internalFormat);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
    glTextureStorage2D(m_handle, mipmapLevelCount, internalFormat, width, height);
    
    SetMinFilter(mipmapLevelCount > 1 ? TextureFilter::LINEAR_MIPMAP : TextureFilter::LINEAR);
    SetMagFilter(TextureFilter::LINEAR);
    SetWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
}

void OpenGLTexture::ClearR(float value) {
    if (!m_handle) return;

    // Allow only non-integer color formats
    if (!(m_format == GL_RED || m_format == GL_RG || m_format == GL_RGB || m_format == GL_RGBA)) {
        std::cout << "OpenGLTexture::ClearR() Unsupported format\n";
        return;
    }

    const GLfloat color[4] = { value, 0.0f, 0.0f, 0.0f };

    for (int level = 0; level < m_mipmapLevelCount; ++level) {
        glClearTexImage(m_handle, level, m_format, GL_FLOAT, color);
    }
}

void OpenGLTexture::UploadData(const float* data) {
    if (!m_handle || !data) return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTextureSubImage2D(m_handle, 0, 0, 0, m_width, m_height, m_format, GL_FLOAT, data);
}

void OpenGLTexture::UploadR16FData(const float* data, int width, int height, int xOffset, int yOffset, int mipLevel) {
    if (!m_handle || !data) return;

    if (m_internalFormat != GL_R16F) {
        Logging::Error() << "UploadR16FData(): failed coz m_internalFormat was not GL_R16F, but was " << OpenGLUtil::GLInternalFormatToString(m_internalFormat) << "(" << m_internalFormat << ")";
        return;
    }

    // Validate bounds against the mip level size
    GLint levelWidth = 0;
    GLint levelHeight = 0;
    glGetTextureLevelParameteriv(m_handle, mipLevel, GL_TEXTURE_WIDTH, &levelWidth);
    glGetTextureLevelParameteriv(m_handle, mipLevel, GL_TEXTURE_HEIGHT, &levelHeight);

    if (xOffset < 0 || yOffset < 0 || xOffset + width  > levelWidth || yOffset + height > levelHeight) {
        Logging::Error() 
            << "UploadR16FData(): out of bounds subimage upload\n"
            << "-xOffset:     " << xOffset << "\n"
            << "-yOffset:     " << yOffset << "\n"
            << "-width:       " << width << "\n"
            << "-height:      " << height << "\n"
            << "-mipLevel:    " << mipLevel << "\n"
            << "-levelWidth:  " << levelWidth << "\n"
            << "-levelHeight: " << levelHeight;
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTextureSubImage2D(m_handle, mipLevel, xOffset, yOffset, width, height, GL_RED, GL_FLOAT, data);
}

void OpenGLTexture::Reset() {
    MakeBindlessTextureNonResident();

    if (m_handle) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
    m_width = 0;
    m_height = 0;
    m_mipmapLevelCount = 0;
    m_bindlessID = 0;
    m_format = 0;
    m_internalFormat = 0;
}

GLuint& OpenGLTexture::GetHandle() {
    return m_handle;
}

int OpenGLTexture::GetWidth() {
    return m_width;
}

int OpenGLTexture::GetHeight() {
    return m_height;
}

int OpenGLTexture::GetChannelCount() {
    return m_channelCount;
}

int OpenGLTexture::GetDataSize() {
    return m_dataSize;
}

size_t OpenGLTexture::GetAllocatedByteCount() const {
    size_t byteCount = 0;
    const size_t bytesPerPixel = GetBytesPerPixel(m_internalFormat);
    const size_t compressedBlockSize = GetCompressedBlockSize(m_internalFormat);

    for (int mipLevel = 0; mipLevel < m_mipmapLevelCount; mipLevel++) {
        const size_t width = static_cast<size_t>(std::max(1, m_width >> mipLevel));
        const size_t height = static_cast<size_t>(std::max(1, m_height >> mipLevel));

        if (compressedBlockSize > 0) {
            const size_t blockCountX = (width + 3) / 4;
            const size_t blockCountY = (height + 3) / 4;
            byteCount += blockCountX * blockCountY * compressedBlockSize;
        }
        else {
            byteCount += width * height * bytesPerPixel;
        }
    }

    return byteCount;
}

void* OpenGLTexture::GetData() {
    return m_data;
}

GLint OpenGLTexture::GetFormat() {
    return m_format;
}
GLint OpenGLTexture::GetInternalFormat() {
    return m_internalFormat;
}

GLint OpenGLTexture::GetMipmapLevelCount() {
    return m_mipmapLevelCount;
}

void OpenGLTexture::SetBorderColor(float r, float g, float b, float a) {
    float borderColor[] = { r, g, b, a };
    glTextureParameterfv(m_handle, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void OpenGLTexture::SetBorderColor(const glm::vec4& color) {
    SetBorderColor(color.r, color.g, color.b, color.a);
}

void OpenGLTexture::SetWrapMode(TextureWrapMode wrapMode) {
    SetWrapModeS(wrapMode);
    SetWrapModeT(wrapMode);
}

void OpenGLTexture::SetWrapModeS(TextureWrapMode wrapMode) {
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, OpenGLUtil::TextureWrapModeToGLEnum(wrapMode));
}

void OpenGLTexture::SetWrapModeT(TextureWrapMode wrapMode) {
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, OpenGLUtil::TextureWrapModeToGLEnum(wrapMode));
}

void OpenGLTexture::SetMinFilter(TextureFilter filter) {
    glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, OpenGLUtil::TextureFilterToGLEnum(filter));
}

void OpenGLTexture::SetMagFilter(TextureFilter filter) {
    glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, OpenGLUtil::TextureFilterToGLEnum(filter));
}

void OpenGLTexture::MakeBindlessTextureResident() {
    if (Hell::BackEnd::RenderDocFound()) return;

    if (m_handle == 0 || glIsTexture(m_handle) != GL_TRUE) {
        Logging::Error() << "OpenGLTexture::MakeBindlessTextureResident() failed: handle '" << m_handle << "' is not a valid texture\n";
        m_bindlessID = 0;
        return;
    }
        
    if (m_bindlessID == 0) {
        m_bindlessID = glGetTextureHandleARB(m_handle);
    }

    if (m_bindlessID == 0) {
        Logging::Error() << "OpenGLTexture::MakeBindlessTextureResident() failed: texture '" << m_handle << "' returned an invalid bindless handle\n";
        return;
    }

    glMakeTextureHandleResidentARB(m_bindlessID);
}

void OpenGLTexture::MakeBindlessTextureNonResident() {
    if (Hell::BackEnd::RenderDocFound()) return;

    if (m_bindlessID != 0) {
        glMakeTextureHandleNonResidentARB(m_bindlessID);
        m_bindlessID = 0;
    }
}
