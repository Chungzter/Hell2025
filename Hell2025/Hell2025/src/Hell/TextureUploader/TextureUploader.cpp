#include "TextureUploader.h"

#include "API/OpenGL/GL_texture_uploader.h"
#include "BackEnd/BackEnd.h"

namespace Hell::TextureUploader {
    bool ImmediateUpload(Texture& texture) {
        if (BackEnd::GetAPI() == API::OPENGL) { return OpenGLTextureUploader::ImmediateUpload(texture); }
        if (BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::ImmediateUpload(texture); */ }

        return false;
    }

    void QueueUpload(Texture& texture) {
        if (BackEnd::GetAPI() == API::OPENGL) { OpenGLTextureUploader::QueueUpload(texture); }
        if (BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::QueueUpload(texture); */ }
    }
}