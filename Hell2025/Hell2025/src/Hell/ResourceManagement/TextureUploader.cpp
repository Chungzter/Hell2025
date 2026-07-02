#include "TextureUploader.h"

#include "Hell/Render/API/OpenGL/GL_texture_uploader.h"
#include "Hell/Backend/BackEnd.h"

namespace Hell::TextureUploader {
    bool ImmediateUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGL::TextureUploader::ImmediateUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /* TODO */ }

        return false;
    }

    void QueueUpload(Texture& texture) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::QueueUpload(texture); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /* TODO */ }
    }

    void Update() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::Update(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /* TODO */ }
    }

    std::vector<Texture*> ConsumeCompletedUploads() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { return OpenGL::TextureUploader::ConsumeCompletedUploads(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /* TODO */ }

        return {};
    }

    void CleanUp() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) { OpenGL::TextureUploader::CleanUp(); }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) { /* TODO */ }
    }
}
