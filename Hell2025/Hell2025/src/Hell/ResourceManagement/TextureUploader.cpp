#include "TextureUploader.h"

#include "Hell/Render/API/OpenGL/GL_texture_uploader.h"
#include "Hell/Backend/BackEnd.h"

namespace Hell::TextureUploader {
    bool ImmediateUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGL::TextureUploader::ImmediateUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::ImmediateUpload(texture); */ }

        return false;
    }

    void QueueUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::QueueUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::QueueUpload(texture); */ }
    }

    void Update() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::Update(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::Update(); */ }
    }

    std::vector<Texture*> ConsumeCompletedUploads() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGL::TextureUploader::ConsumeCompletedUploads(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*return VulkanTextureUploader::ConsumeCompletedUploads(); */ }

        return {};
    }

    void CleanUp() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::CleanUp(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /*VulkanTextureUploader::CleanUp(); */ }
    }
}
