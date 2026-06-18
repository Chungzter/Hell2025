#include "ImageTools.h"

#include <iostream>

namespace ImageTools {

    void ConvertRGBA8ToR16F(ImageData& imageData) {
        if (imageData.format != ImageFormat::RGBA8_UNORM) {
            std::cout << "ConvertRGBA8ToR16F() requires RGBA8_UNORM image data\n";
            return;
        }

        for (TextureMip& mip : imageData.mips) {
            const size_t pixelCount = static_cast<size_t>(mip.width) * mip.height;
            if (mip.data.size() != pixelCount * 4) {
                std::cout << "ConvertRGBA8ToR16F() encountered an invalid mip payload\n";
                return;
            }

            const uint8_t* rgbaData = reinterpret_cast<const uint8_t*>(mip.data.data());
            std::vector<std::byte> halfFloatData(pixelCount * sizeof(uint16_t));
            uint16_t* redData = reinterpret_cast<uint16_t*>(halfFloatData.data());

            for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
                redData[pixelIndex] = FloatToHalf(rgbaData[pixelIndex * 4] / 255.0f);
            }

            mip.data = std::move(halfFloatData);
        }

        imageData.format = ImageFormat::R16_SFLOAT;
        imageData.type = ImageDataType::UNCOMPRESSED;
    }

    bool IsEightBitImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R8_UNORM:
        case ImageFormat::RG8_UNORM:
        case ImageFormat::RGB8_UNORM:
        case ImageFormat::RGBA8_UNORM:
        case ImageFormat::RGB8_SRGB:
        case ImageFormat::RGBA8_SRGB:
            return true;
        default:
            return false;
        }
    }

    bool IsHalfFloatImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R16_SFLOAT:
        case ImageFormat::RG16_SFLOAT:
        case ImageFormat::RGB16_SFLOAT:
        case ImageFormat::RGBA16_SFLOAT:
            return true;
        default:
            return false;
        }
    }

    bool IsFullFloatImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R32_SFLOAT:
        case ImageFormat::RG32_SFLOAT:
        case ImageFormat::RGB32_SFLOAT:
        case ImageFormat::RGBA32_SFLOAT:
            return true;
        default:
            return false;
        }
    }

    float HalfToFloat(uint16_t h) {
        uint32_t sign = (h & 0x8000) << 16;      // Extract sign bit
        uint32_t exponent = (h & 0x7C00) >> 10;  // Extract exponent
        uint32_t mantissa = (h & 0x03FF);        // Extract mantissa

        if (exponent == 0) {
            if (mantissa == 0) {
                float zero = (sign != 0) ? -0.0f : 0.0f;
                return zero;
            }
            float subnormal = std::ldexp(static_cast<float>(mantissa), -24);
            return subnormal;
        }

        if (exponent == 31) {
            float special = (mantissa == 0) ? std::copysign(INFINITY, sign) : NAN;
            return special;
        }

        exponent = exponent - 15 + 127;  // Convert exponent from half-float to full-float
        uint32_t floatBits = sign | (exponent << 23) | (mantissa << 13);

        float result;
        std::memcpy(&result, &floatBits, sizeof(result));  // Bitcast to float

        return result;
    }

    uint16_t FloatToHalf(float value) {
        uint32_t f = *(uint32_t*)&value; // Interpret float as uint32_t
        uint32_t sign = (f >> 16) & 0x8000; // Extract sign bit
        uint32_t exponent = (f >> 23) & 0xFF; // Extract exponent
        uint32_t mantissa = f & 0x7FFFFF; // Extract mantissa

        if (exponent == 255) { // Inf or NaN
            if (mantissa) return sign | 0x7E00; // NaN
            return sign | 0x7C00; // Infinity
        }

        if (exponent > 142) { // Too large, return infinity
            return sign | 0x7C00;
        }

        if (exponent < 113) { // Too small, round to zero or denormalized
            if (exponent < 103) return sign; // Underflow to zero
            mantissa |= 0x800000; // Set implicit leading 1
            mantissa >>= (113 - exponent); // Shift to denormalized range
            return sign | (mantissa >> 13);
        }

        return sign | ((exponent - 112) << 10) | (mantissa >> 13);
    }
}