#include "Texture.h"

#include "AssetManagement/AssetManager.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Util/Util.h"

#include <iostream> // TODO clean up logging

using namespace Hell;

void Texture::Load() {
    // Load texture data from disk
    if (m_imageDataType == ImageDataType::UNCOMPRESSED) {
        m_imageData = ImageTools::LoadUncompressedImage(m_fileInfo.path);
    }
    else if (m_imageDataType == ImageDataType::COMPRESSED) {
        m_imageData = ImageTools::LoadDDS(m_fileInfo.path);
    }
    else if (m_imageDataType == ImageDataType::EXR) {
        m_imageData = ImageTools::LoadEXRImage(m_fileInfo.path);
    }
    m_loadingState = LoadingState::Value::LOADING_COMPLETE;

    // Calculate mipmap level count
    if (!m_imageData.mips.empty()) {
        m_mipmapLevelCount = 1 + static_cast<int>(std::log2(std::max(GetWidth(), GetHeight())));
    }
    else {
        m_mipmapLevelCount = 0;
    }

    // Initiate bake states
    m_textureDataLevelBakeStates.resize(m_imageData.mips.size(), BakeState::AWAITING_BAKE);
}

void Texture::FreeCPUMemory() {
    for (TextureMip& mip : m_imageData.mips) {
        mip.data.clear();
        mip.data.shrink_to_fit();
    }
}

const int Texture::GetWidth() {
    return GetMipMapWidth(0);
}

const int Texture::GetHeight() {
    return GetMipMapHeight(0);
}

const int Texture::GetMipMapWidth(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].width;
    }
    else {
        std::cout << "Texture::GetMipMapWidth(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const int Texture::GetMipMapHeight(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].height;
    }
    else {
        std::cout << "Texture::GetMipMapHeight(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const void* Texture::GetData(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].data.data();
    }
    else {
        std::cout << "Texture::GetData(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return nullptr;
    }
}

const int Texture::GetDataSize(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return static_cast<int>(m_imageData.mips[mipmapLevel].data.size());
    }
    else {
        std::cout << "Texture::GetDataSize(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const int Texture::GetChannelCount() {
    return GetImageFormatChannelCount(m_imageData.format);
}

void Texture::SetTextureDataLevelBakeState(int index, BakeState state) {
    if (index >= 0 && m_textureDataLevelBakeStates.size() && index < m_textureDataLevelBakeStates.size()) {
        m_textureDataLevelBakeStates[index] = state;
    }
    else {
        std::cout << "Texture::SetTextureDataLevelBakeState(int index, BakeState state) failed. Index '" << index << "' out of range of size " << m_textureDataLevelBakeStates.size() << "\n";
    }
}

void Texture::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

void Texture::SetImageDataType(ImageDataType imageDataType) {
    m_imageDataType = imageDataType;
}

void Texture::SetLoadingState(LoadingState loadingState) {
    m_loadingState = loadingState;
}

void Texture::SetTextureWrapMode(TextureWrapMode wrapMode) {
    m_wrapMode = wrapMode;
}

void Texture::SetBorderColor(float r, float g, float b, float a) {
    m_borderColor = glm::vec4(r, g, b, a);
}

void Texture::SetMinFilter(TextureFilter filter) {
    m_minFilter = filter;
}

void Texture::SetMagFilter(TextureFilter filter) {
    m_magFilter = filter;
}

const BakeState Texture::GetTextureDataLevelBakeState(int index) {
    if (index >= 0 && m_textureDataLevelBakeStates.size() && index < m_textureDataLevelBakeStates.size()) {
        return m_textureDataLevelBakeStates[index];
    }
    else {
        std::cout << "Texture::GetTextureDataLevelBakeState(int index) failed. Index '" << index << "' out of range of size " << m_textureDataLevelBakeStates.size() << "\n";
        return BakeState::UNDEFINED;
    }
}

void Texture::CheckForBakeCompletion() {
    if (m_bakeComplete) {
        return;
    }
    else {
        m_bakeComplete = true;
        for (BakeState& state : m_textureDataLevelBakeStates) {
            if (state != BakeState::BAKE_COMPLETE) {
                m_bakeComplete = false;
                return;
            }
        }
        // Bake is complete!
        AssetManager::AddItemToLoadLog(GetFilePath());
    }
}

const bool Texture::BakeComplete() {
    return m_bakeComplete;
}

const int Texture::GetTextureDataCount() {
    return m_imageData.mips.size();
}

void Texture::RequestMipmaps() {
    m_mipmapsRequested = true;
}

const bool Texture::MipmapsAreRequested() {
    return m_mipmapsRequested;
}

const void Texture::PrintDebugInfo() {
    std::cout << GetFileName() << "\n";
    std::cout << " - width: " << GetWidth() << "\n";
    std::cout << " - height: " << GetHeight() << "\n";
    std::cout << " - channel count: " << GetChannelCount() << "\n";
    std::cout << " - format: " << ImageFormatToString(GetImageFormat()) << "\n";
    std::cout << " - mipmap level count: " << GetMipmapLevelCount() << "\n";
    std::cout << " - mipmaps requested: " << (MipmapsAreRequested() ? "TRUE" : "FALSE") << "\n";
    std::cout << " - data size:\n";
    for (int i = 0; i < GetTextureDataCount(); i++) {
        std::cout << "   mip " << i << " " << GetDataSize(i) << " at " << GetData(i) << "\n";
    }
    std::cout << "\n";
}
