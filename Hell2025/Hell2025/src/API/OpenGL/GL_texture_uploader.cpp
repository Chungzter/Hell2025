#include "GL_texture_uploader.h"

#include "API/OpenGL/GL_util.h"
#include "Hell/Logging.h"

#include <limits>

namespace OpenGLTextureUploader {

    bool ImmediateUpload(Texture& texture) {
        const ImageData& imageData = texture.GetImageData();
        if (imageData.mips.empty()) {
            Logging::Error() << "OpenGLTextureUploader::ImmediateUpload(..) failed because texture '" << texture.GetFileName() << "' has no image data\n";
            return false;
        }

        const TextureMip& baseMip = imageData.mips[0];
        if (baseMip.width == 0 || baseMip.height == 0 || baseMip.data.empty()) {
            Logging::Error() << "OpenGLTextureUploader::ImmediateUpload(..) failed because texture '" << texture.GetFileName() << "' has invalid base mip data\n";
            return false;
        }

        const GLenum format = OpenGLUtil::ImageFormatToGLFormat(imageData.format);
        const GLenum internalFormat = OpenGLUtil::ImageFormatToGLInternalFormat(imageData.format);
        const GLenum dataType = OpenGLUtil::ImageFormatToGLDataType(imageData.format);

        if (format == GL_NONE || internalFormat == GL_NONE) {
            Logging::Error() << "OpenGLTextureUploader::ImmediateUpload(..) failed because texture '" << texture.GetFileName() << "' has an unsupported image format\n";
            return false;
        }

        const bool generateMipmaps = texture.MipmapsAreRequested() && imageData.mips.size() == 1;
        const int allocatedMipCount = generateMipmaps ? texture.GetMipmapLevelCount() : (texture.MipmapsAreRequested() ? static_cast<int>(imageData.mips.size()) : 1);
        const size_t uploadMipCount = texture.MipmapsAreRequested() ? imageData.mips.size() : 1;

        OpenGLTexture& glTexture = texture.GetGLTexture();
        glTexture.Create(static_cast<int>(baseMip.width), static_cast<int>(baseMip.height), internalFormat, allocatedMipCount);
        glTexture.SetWrapModeS(texture.GetTextureWrapModeS());
        glTexture.SetWrapModeT(texture.GetTextureWrapModeT());
        glTexture.SetMinFilter(texture.GetMinFilter());
        glTexture.SetMagFilter(texture.GetMagFilter());
        glTexture.SetBorderColor(texture.GetBorderColor());

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
            const TextureMip& mip = imageData.mips[mipIndex];

            if (mip.width == 0 || mip.height == 0 || mip.data.empty()) {
                Logging::Error() << "OpenGLTextureUploader::ImmediateUpload(..) failed because texture '" << texture.GetFileName() << "' has invalid mip " << mipIndex << "\n";
                return false;
            }

            if (mip.data.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max())) {
                Logging::Error() << "OpenGLTextureUploader::ImmediateUpload(..) failed because mip " << mipIndex << " of texture '" << texture.GetFileName() << "' is too large\n";
                return false;
            }

            if (IsCompressedImageFormat(imageData.format)) {
                glCompressedTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), internalFormat, static_cast<GLsizei>(mip.data.size()), mip.data.data());
            }
            else {
                glTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), format, dataType, mip.data.data());
            }
        }

        if (generateMipmaps) {
            glGenerateTextureMipmap(glTexture.GetHandle());
        }

        glTexture.MakeBindlessTextureResident();

        for (int mipIndex = 0; mipIndex < texture.GetTextureDataCount(); ++mipIndex) {
            texture.SetTextureDataLevelBakeState(mipIndex, BakeState::BAKE_COMPLETE);
        }

        texture.CheckForBakeCompletion();

        return true;
    }

    void QueueUpload(Texture& texture) {

    }
}
