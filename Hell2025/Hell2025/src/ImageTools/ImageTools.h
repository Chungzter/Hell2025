#pragma once

#include "Hell/TextureTypes.h"

#include <string>
#include <vector>

enum class CompressionType { DXT3, BC5, UNDEFINED };

namespace ImageTools {
    // Compressor
    std::string CMPErrorToString(int error);
    std::string CMPFormatToString(int format);

    // Compresesenator
    void InitializeCMPFramework();
    bool IsCMPFrameworkInitialized();
    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool createMipMaps);

    // Image Data
    ImageData LoadDDS(const std::string& filepath);
    ImageData LoadUncompressedImage(const std::string& filepath);
    ImageData LoadR16F(const std::string& filepath);
    ImageData LoadEXRImage(const std::string& filepath);

    // Writing
    void SaveBitmap(const char* filename, unsigned char* data, int width, int height, int numChannels); // problematticly similar to below!!!!
    void SaveBitmap(const std::string& filename, const void* data, int width, int height, ImageFormat format);
    void SaveHeightMapR16F(const std::string& filename, void* data, int width, int height);
    void SaveTextureAsBitmap(const std::vector<std::vector<uint16_t>>& pixels, int width, int height, const std::string& filename);
    void SaveFloatArrayTextureAsBitmap(const std::vector<float>& data, int width, int height, ImageFormat format, const std::string& filename);

    // Util
    void ConvertRGBA8ToR16F(ImageData& imageData);
    bool IsEightBitImageFormat(ImageFormat format);
    bool IsHalfFloatImageFormat(ImageFormat format);
    bool IsFullFloatImageFormat(ImageFormat format);
    int GetChannelCountFromCMPFormat(int format);
    float HalfToFloat(uint16_t h);
    uint16_t FloatToHalf(float value);
}
