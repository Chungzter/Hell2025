#pragma once

#include "API/OpenGL/Types/gl_texture.h"
#include "API/Vulkan/Types/vk_texture.h"

#include "Hell/Enums.h"
#include "Hell/TextureTypes.h"

#include "LoadingState.h"
#include <string>
#include <memory>

struct Texture {
public:
    Texture() = default;
    void Load();
    void SetLoadingState(LoadingState value);
    void SetFileInfo(FileInfo fileInfo);
    void SetImageDataType(ImageDataType imageDataType);
    void SetTextureWrapMode(TextureWrapMode wrapMode);
    void SetBorderColor(float r, float g, float b, float a);
    void SetMinFilter(TextureFilter filter);
    void SetMagFilter(TextureFilter filter);
    void SetTextureDataLevelBakeState(int index, BakeState state);
    void RequestMipmaps();
    void FreeCPUMemory();
    const void PrintDebugInfo();
    void CheckForBakeCompletion();
    const bool MipmapsAreRequested();
    const bool BakeComplete();
    const int GetTextureDataCount();
    const int GetWidth();
    const int GetHeight();
    const int GetMipMapWidth(int mipmapLevel);
    const int GetMipMapHeight(int mipmapLevel);
    const int GetFormat();
    const int GetInternalFormat();
    const int GetDataSize(int mipmapLevel);
    const int GetChannelCount();
    const void* GetData(int mipmapLevel);
    const BakeState GetTextureDataLevelBakeState(int index);

    OpenGLTexture& GetGLTexture() { return m_glTexture; }
    VulkanTexture& GetVKTexture() { return m_vkTexture; }

    const int GetMipmapLevelCount()                  { return m_mipmapLevelCount; }
    const std::string& GetFileName()                 { return m_fileInfo.name; }
    const std::string& GetFilePath()                 { return m_fileInfo.path; }
    const FileInfo GetFileInfo() const               { return m_fileInfo; }
    const ImageDataType GetImageDataType() const     { return m_imageDataType; }
    const TextureWrapMode GetTextureWrapMode() const { return m_wrapMode; }
    const TextureFilter GetMinFilter() const         { return m_minFilter; }
    const TextureFilter GetMagFilter() const         { return m_magFilter; }
    const glm::vec4& GetBorderColor() const          { return m_borderColor; }
    LoadingState GetLoadingState() const             { return m_loadingState; }

private:
    OpenGLTexture m_glTexture;
    VulkanTexture m_vkTexture;
    LoadingState m_loadingState { LoadingState::Value::AWAITING_LOADING_FROM_DISK };
    ImageDataType m_imageDataType = ImageDataType::UNDEFINED;
    TextureWrapMode m_wrapMode = TextureWrapMode::REPEAT;
    TextureFilter m_minFilter = TextureFilter::NEAREST;
    TextureFilter m_magFilter = TextureFilter::NEAREST;
    FileInfo m_fileInfo;
    std::vector<TextureData> m_textureDataLevels;
    std::vector<BakeState> m_textureDataLevelBakeStates;
    int m_mipmapLevelCount = 0;
    bool m_mipmapsRequested = false;
    bool m_bakeComplete = false;
    glm::vec4 m_borderColor = glm::vec4(0.0f);
};
