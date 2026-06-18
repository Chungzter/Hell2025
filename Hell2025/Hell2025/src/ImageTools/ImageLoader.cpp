#include "ImageTools.h"
#include "DDS.h"

#include <stb_image.h>
#include <tinyexr.h>

#include <fstream>

namespace ImageTools {

    ImageFormat GetUncompressedImageFormat(int channelCount) {
        switch (channelCount) {
        case 1: return ImageFormat::R8_UNORM;
        case 2: return ImageFormat::RG8_UNORM;
        case 3: return ImageFormat::RGB8_UNORM;
        case 4: return ImageFormat::RGBA8_UNORM;
        default: return ImageFormat::UNDEFINED;
        }
    }

    ImageData LoadDDS(const std::string& filepath) {
        ImageData imageData;
        imageData.type = ImageDataType::COMPRESSED;

        // Open the file in binary mode
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            std::cout << "Failed to open DDS file: " << filepath << "\n";
            return imageData;
        }
        // Read and validate the DDS header
        DDSHeader header = {};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.dwMagic != 0x20534444) { // "DDS " magic number
            std::cout << "Not a valid DDS file: " << filepath << "\n";
            return imageData;
        }
        // Check for potential DX10 extended header
        DDSHeaderDX10 dx10Header = {};
        if (header.ddspf_dwFourCC == FOURCC_DX10) {
            file.read(reinterpret_cast<char*>(&dx10Header), sizeof(dx10Header));
            if (!file) {
                std::cout << "Failed to read DDS DX10 header: " << filepath << "\n";
                return imageData;
            }
        }
        // Retrieve format information
        DDSFormatInfo formatInfo = GetDDSFormatInfo(header, &dx10Header);
        if (formatInfo.format == ImageFormat::UNDEFINED || formatInfo.blockSize == 0) {
            return imageData;
        }
        imageData.format = formatInfo.format;

        // Iterate the mipmap levels
        uint32_t mipWidth = header.dwWidth;
        uint32_t mipHeight = header.dwHeight;
        const uint32_t mipCount = std::max(1u, header.dwMipMapCount);
        imageData.mips.reserve(mipCount);
        for (uint32_t i = 0; i < mipCount; ++i) {
            uint32_t blocksWide = (mipWidth + 3) / 4;
            uint32_t blocksHigh = (mipHeight + 3) / 4;
            uint32_t dataSize = blocksWide * blocksHigh * formatInfo.blockSize;

            TextureMip mip;
            mip.width = mipWidth;
            mip.height = mipHeight;
            mip.data.resize(dataSize);
            file.read(reinterpret_cast<char*>(mip.data.data()), dataSize);
            if (file.gcount() != static_cast<std::streamsize>(dataSize)) {
                std::cerr << "Error reading mip level " << i << "\n";
                break;
            }
            imageData.mips.push_back(std::move(mip));
            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        return imageData;
    }

    ImageData LoadUncompressedImage(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(false);
        ImageData imageData;
        imageData.type = ImageDataType::UNCOMPRESSED;

        int width = 0;
        int height = 0;
        int channelCount = 0;
        uint8_t* pixels = stbi_load(filepath.c_str(), &width, &height, &channelCount, 0);
        if (!pixels) {
            std::cout << "Failed to load image: " << filepath << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;

        if (channelCount == 3) {
            const size_t newSize = static_cast<size_t>(width) * height * 4;
            mip.data.resize(newSize);
            uint8_t* rgbaData = reinterpret_cast<uint8_t*>(mip.data.data());
            for (size_t i = 0, j = 0; i < newSize; i += 4, j += 3) {
                rgbaData[i] = pixels[j];         // R
                rgbaData[i + 1] = pixels[j + 1]; // G
                rgbaData[i + 2] = pixels[j + 2]; // B
                rgbaData[i + 3] = 255;              // A
            }
            imageData.format = ImageFormat::RGBA8_UNORM;
        }
        else {
            const size_t dataSize = static_cast<size_t>(width) * height * channelCount;
            mip.data.resize(dataSize);
            std::memcpy(mip.data.data(), pixels, dataSize);
            imageData.format = GetUncompressedImageFormat(channelCount);
        }

        stbi_image_free(pixels);
        return imageData;
    }

    ImageData LoadEXRImage(const std::string& filepath) {
        ImageData imageData;
        imageData.type = ImageDataType::EXR;
        const char* err = nullptr;
        float* pixels = nullptr;
        int width = 0;
        int height = 0;
        const int status = LoadEXR(&pixels, &width, &height, filepath.c_str(), &err);
        if (status != TINYEXR_SUCCESS) {
            std::cout << "Failed to load EXR: " << filepath;
            if (err) {
                std::cout << " (" << err << ")";
                FreeEXRErrorMessage(err);
            }
            std::cout << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;
        mip.data.resize(static_cast<size_t>(width) * height * 4 * sizeof(float));
        std::memcpy(mip.data.data(), pixels, mip.data.size());
        free(pixels);

        imageData.format = ImageFormat::RGBA32_SFLOAT;
        return imageData;
    }

    ImageData LoadR16F(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(false);
        ImageData imageData;
        imageData.type = ImageDataType::UNCOMPRESSED;

        int width, height, channels;
        uint16_t* pixels = stbi_load_16(filepath.c_str(), &width, &height, &channels, 1); // Force single-channel

        if (!pixels) {
            std::cout << "[LoadR16FTextureData] Failed to load 16-bit texture: " << filepath << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;
        mip.data.resize(static_cast<size_t>(width) * height * sizeof(uint16_t));
        std::memcpy(mip.data.data(), pixels, mip.data.size());
        stbi_image_free(pixels);

        // stb_image returns normalized unsigned 16-bit samples, not IEEE half floats.
        imageData.format = ImageFormat::R16_UNORM;
        return imageData;
    }
}