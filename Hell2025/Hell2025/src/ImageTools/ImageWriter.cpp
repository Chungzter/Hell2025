#include "ImageTools.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include "stb_image_write.h"
#pragma warning(pop)

#include <lodepng/lodepng.h>

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace ImageTools {

    void CreateFolder(const char* path) {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir)) {
            if (!std::filesystem::create_directories(dir) && !std::filesystem::exists(dir)) {
                std::cout << "Failed to create directory: " << path << "\n";
            }
        }
    }

    void SaveBitmap(const char* filename, unsigned char* data, int width, int height, int numChannels) {
        unsigned char* flippedData = (unsigned char*)malloc(width * height * numChannels);
        if (!flippedData) {
            std::cerr << "[ERROR] Failed to allocate memory for flipped data\n";
            return;
        }
        for (int y = 0; y < height; ++y) {
            std::memcpy(flippedData + (height - y - 1) * width * numChannels,
                data + y * width * numChannels,
                width * numChannels);
        }
        if (stbi_write_bmp(filename, width, height, numChannels, flippedData)) {
            std::cout << "Saved bitmap: " << filename << "\n";
        }
        else {
            std::cerr << "Failed to save bitmap: " << filename << "\n";
        }
        free(flippedData);
    }

    // Possibly broken!!!
    // Possibly broken!!!
    // Possibly broken!!!
    // Possibly broken!!!
    // Possibly broken!!!
    void SaveTextureAsBitmap(const std::vector<std::vector<uint16_t>>& pixels, int width, int height, const std::string& filename) {
        std::vector<uint8_t> imageData(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                imageData[y * width + x] = static_cast<uint8_t>(pixels[x][y] & 0xFF); // Clamp to 8-bit gray scale
            }
        }

        // Save as BMP (1-channel grayscale)
        if (stbi_write_bmp(filename.c_str(), width, height, 1, imageData.data())) {
            std::cout << "Saved " << filename << " successfully!\n";
        }
        else {
            std::cout << "Failed to save " << filename << "\n";
        }
    }

    void SaveNormalizedFloatDataAsBitmap(const std::vector<float>& data, int width, int height, int channelCount, const std::string& filename) {
        std::vector<uint8_t> outputData(static_cast<size_t>(width) * height * 3);

        for (size_t pixelIndex = 0; pixelIndex < static_cast<size_t>(width)* height; ++pixelIndex) {
            const size_t componentIndex = pixelIndex * channelCount;
            float r = data[componentIndex];
            float g = channelCount > 1 ? data[componentIndex + 1] : r;
            float b = channelCount > 2 ? data[componentIndex + 2] : (channelCount == 1 ? r : 0.0f);

            outputData[pixelIndex * 3 + 0] = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
            outputData[pixelIndex * 3 + 1] = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
            outputData[pixelIndex * 3 + 2] = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
        }

        if (!stbi_write_bmp(filename.c_str(), width, height, 3, outputData.data())) {
            std::cout << "Error: Failed to save BMP file!\n";
        }
        else {
            std::cout << "Saved BMP successfully: " << filename << "\n";
        }
    }

    void SaveBitmap(const std::string& filename, const void* data, int width, int height, ImageFormat format) {
        if (!data || width <= 0 || height <= 0) {
            std::cout << "SaveBitmap() failed: invalid image data or dimensions\n";
            return;
        }

        const int channelCount = GetImageFormatChannelCount(format);
        if (channelCount == 0 || IsCompressedImageFormat(format)) {
            std::cout << "SaveBitmap() failed: Unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        const size_t componentCount = static_cast<size_t>(width) * height * channelCount;
        std::vector<float> floatData(componentCount);

        if (IsEightBitImageFormat(format)) {
            const uint8_t* source = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = source[i] / 255.0f;
            }
        }
        else if (format == ImageFormat::R16_UNORM) {
            const uint16_t* source = static_cast<const uint16_t*>(data);
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = source[i] / 65535.0f;
            }
        }
        else if (IsHalfFloatImageFormat(format)) {
            const uint16_t* source = static_cast<const uint16_t*>(data);
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = HalfToFloat(source[i]);
            }
        }
        else if (IsFullFloatImageFormat(format)) {
            const float* source = static_cast<const float*>(data);
            floatData.assign(source, source + componentCount);
        }
        else {
            std::cout << "SaveBitmap() failed: Unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        SaveNormalizedFloatDataAsBitmap(floatData, width, height, channelCount, filename);
    }

    void SaveHeightMapR16F(const std::string& filename, void* data, int width, int height) {
        if (!data) {
            std::cerr << "Error: Data pointer is null, cannot save PNG.\n";
            return;
        }

        uint16_t* rawData = static_cast<uint16_t*>(data);
        size_t pixelCount = width * height;
        std::vector<uint16_t> outputData(pixelCount);

        // Find min/max values
        uint16_t minVal = 65535, maxVal = 0;
        for (size_t i = 0; i < pixelCount; ++i) {
            minVal = std::min(minVal, rawData[i]);
            maxVal = std::max(maxVal, rawData[i]);
        }

        std::cout << "[SaveHeightMapR16F] Min: " << minVal << ", Max: " << maxVal << "\n";

        // Avoid divide-by-zero when normalizing
        float range = (maxVal - minVal) > 0 ? (maxVal - minVal) : 1.0f;
        std::cout << "[SaveHeightMapR16F] Normalization range: " << range << "\n";

        // Normalize and scale to 16-bit
        for (size_t i = 0; i < pixelCount; ++i) {
            //outputData[i] = static_cast<uint16_t>(((rawData[i] - minVal) / range) * 65535.0f);
            outputData[i] = rawData[i];
        }

        // Print first 10 pixel values BEFORE saving
        std::cout << "[SaveHeightMapR16F] First 10 pixel values before saving:\n";
        for (int i = 0; i < 10; ++i) {
            std::cout << "Pixel[" << i << "]: Raw=" << rawData[i]
                << " -> Normalized=" << outputData[i] << "\n";
        }

        // Encode directly without byte swapping
        std::vector<unsigned char> png;
        unsigned error = lodepng::encode(png, reinterpret_cast<const unsigned char*>(outputData.data()), width, height, LCT_GREY, 16);

        if (error) {
            std::cerr << "[SaveHeightMapR16F] LodePNG error: " << lodepng_error_text(error) << "\n";
            return;
        }

        lodepng::save_file(png, filename);
        std::cout << "[SaveHeightMapR16F] Saved 16-bit grayscale PNG successfully: " << filename << "\n";
    }

    void SaveFloatArrayTextureAsBitmap(const std::vector<float>& data, int width, int height, ImageFormat format, const std::string& filename) {
        if (data.empty()) {
            std::cout << "SaveTextureAsBitmap() failed: data was empty\n";
            return;
        }

        const int channelCount = GetImageFormatChannelCount(format);
        if (channelCount == 0 || IsCompressedImageFormat(format)) {
            std::cout << "SaveTextureAsBitmap() failed: Unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        const size_t expectedSize = static_cast<size_t>(width) * height * channelCount;
        if (data.size() != expectedSize) {
            std::cout << "SaveTextureAsBitmap() failed: Data size mismatch. Expected " << expectedSize << ", got " << data.size() << "\n";
            return;
        }

        std::vector<float> normalizedData = data;
        if (IsEightBitImageFormat(format)) {
            for (float& value : normalizedData) {
                value /= 255.0f;
            }
        }
        else if (format == ImageFormat::R16_UNORM) {
            for (float& value : normalizedData) {
                value /= 65535.0f;
            }
        }

        SaveNormalizedFloatDataAsBitmap(normalizedData, width, height, channelCount, filename);
    }
}