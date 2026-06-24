#include "TextureUploader.h"

#include "API/OpenGL/GL_texture_uploader.h"
#include "Hell/Backend/BackEnd.h"

namespace Hell::TextureUploader {
    bool ImmediateUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGLTextureUploader::ImmediateUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::ImmediateUpload(texture); */ }

        return false;
    }

    void QueueUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGLTextureUploader::QueueUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::QueueUpload(texture); */ }
    }

    void Update() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGLTextureUploader::Update(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::Update(); */ }
    }

    std::vector<Texture*> ConsumeCompletedUploads() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGLTextureUploader::ConsumeCompletedUploads(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*return VulkanTextureUploader::ConsumeCompletedUploads(); */ }

        return {};
    }

    void CleanUp() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGLTextureUploader::CleanUp(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::CleanUp(); */ }
    }
}
