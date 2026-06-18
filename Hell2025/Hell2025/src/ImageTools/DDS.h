#pragma once

#include "Hell/TextureTypes.h"

#include <cstdint>
#include <iostream>

#define FOURCC_DXT1 0x31545844 // "DXT1"
#define FOURCC_DXT3 0x33545844 // "DXT3"
#define FOURCC_DXT5 0x35545844 // "DXT5"
#define FOURCC_DX10 0x30315844 // "DX10"
#define FOURCC_ATI1 0x31495441 // "ATI1"
#define FOURCC_ATI2 0x32495441 // "ATI2"
#define FOURCC_BC4U 0x55344342 // "BC4U"
#define FOURCC_BC5U 0x55354342 // "BC5U"

struct DDSHeader {
    uint32_t dwMagic;
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    uint32_t ddspf_dwSize;
    uint32_t ddspf_dwFlags;
    uint32_t ddspf_dwFourCC;
    uint32_t ddspf_dwRGBBitCount;
    uint32_t ddspf_dwRBitMask;
    uint32_t ddspf_dwGBitMask;
    uint32_t ddspf_dwBBitMask;
    uint32_t ddspf_dwABitMask;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

struct DDSHeaderDX10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t reserved;
};

struct DDSFormatInfo {
    ImageFormat format = ImageFormat::UNDEFINED;
    uint32_t blockSize = 0;
};

inline DDSFormatInfo GetDDSFormatInfo(const DDSHeader& header, const DDSHeaderDX10* dx10Header) {
    DDSFormatInfo formatInfo = {};
    if (header.ddspf_dwFourCC == FOURCC_DXT1) {
        formatInfo.format = header.ddspf_dwABitMask
            ? ImageFormat::BC1_RGBA_UNORM
            : ImageFormat::BC1_RGB_UNORM;
        formatInfo.blockSize = 8;
    }
    else if (header.ddspf_dwFourCC == FOURCC_DXT3) {
        formatInfo.format = ImageFormat::BC2_RGBA_UNORM;
        formatInfo.blockSize = 16;
    }
    else if (header.ddspf_dwFourCC == FOURCC_DXT5) {
        formatInfo.format = ImageFormat::BC3_RGBA_UNORM;
        formatInfo.blockSize = 16;
    }
    else if (header.ddspf_dwFourCC == FOURCC_ATI1 || header.ddspf_dwFourCC == FOURCC_BC4U) {
        formatInfo.format = ImageFormat::BC4_R_UNORM;
        formatInfo.blockSize = 8;
    }
    else if (header.ddspf_dwFourCC == FOURCC_ATI2 || header.ddspf_dwFourCC == FOURCC_BC5U) {
        formatInfo.format = ImageFormat::BC5_RG_UNORM;
        formatInfo.blockSize = 16;
    }
    else if (header.ddspf_dwFourCC == FOURCC_DX10 && dx10Header) {
        switch (dx10Header->dxgiFormat) {
            case 71: formatInfo = { ImageFormat::BC1_RGBA_UNORM, 8 }; break;  // DXGI_FORMAT_BC1_UNORM
            case 72: formatInfo = { ImageFormat::BC1_RGBA_SRGB, 8 }; break;   // DXGI_FORMAT_BC1_UNORM_SRGB
            case 74: formatInfo = { ImageFormat::BC2_RGBA_UNORM, 16 }; break; // DXGI_FORMAT_BC2_UNORM
            case 75: formatInfo = { ImageFormat::BC2_RGBA_SRGB, 16 }; break;  // DXGI_FORMAT_BC2_UNORM_SRGB
            case 77: formatInfo = { ImageFormat::BC3_RGBA_UNORM, 16 }; break; // DXGI_FORMAT_BC3_UNORM
            case 78: formatInfo = { ImageFormat::BC3_RGBA_SRGB, 16 }; break;  // DXGI_FORMAT_BC3_UNORM_SRGB
            case 80: formatInfo = { ImageFormat::BC4_R_UNORM, 8 }; break;     // DXGI_FORMAT_BC4_UNORM
            case 83: formatInfo = { ImageFormat::BC5_RG_UNORM, 16 }; break;   // DXGI_FORMAT_BC5_UNORM
            case 95: formatInfo = { ImageFormat::BC6H_RGB_UFLOAT, 16 }; break;// DXGI_FORMAT_BC6H_UF16
            case 96: formatInfo = { ImageFormat::BC6H_RGB_SFLOAT, 16 }; break;// DXGI_FORMAT_BC6H_SF16
            case 98: formatInfo = { ImageFormat::BC7_RGBA_UNORM, 16 }; break; // DXGI_FORMAT_BC7_UNORM
            case 99: formatInfo = { ImageFormat::BC7_RGBA_SRGB, 16 }; break;  // DXGI_FORMAT_BC7_UNORM_SRGB
            default:
                std::cerr << "Unsupported DX10 format: " << dx10Header->dxgiFormat << "\n";
                return {};
        }
    }
    else {
        std::cerr << "Unsupported DDS format: " << header.ddspf_dwFourCC << "\n";
        return {};
    }
    return formatInfo;
}
