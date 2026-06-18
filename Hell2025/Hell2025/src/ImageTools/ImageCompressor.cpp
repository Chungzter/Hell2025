#include "ImageTools.h"

#include "cmp_compressonatorlib/compressonator.h"

#include <iostream>

namespace ImageTools {

    bool m_CMPFrameworkInitilized = false;

    void InitializeCMPFramework() {
        CMP_InitFramework();
        m_CMPFrameworkInitilized = true;
    }

    bool IsCMPFrameworkInitialized() {
        return m_CMPFrameworkInitilized;
    }

    bool CompressionCallback(CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
        (pUser1);
        (pUser2);
        std::printf("\rCompression progress = %3.0f", fProgress);
        return false;
    }

    ImageFormat CMPFormatToImageFormat(CMP_FORMAT format) {
        switch (format) {
        case CMP_FORMAT_DXT1:
        case CMP_FORMAT_BC1: return ImageFormat::BC1_RGB_UNORM;
        case CMP_FORMAT_DXT3:
        case CMP_FORMAT_BC2: return ImageFormat::BC2_RGBA_UNORM;
        case CMP_FORMAT_DXT5:
        case CMP_FORMAT_BC3: return ImageFormat::BC3_RGBA_UNORM;
        case CMP_FORMAT_BC4: return ImageFormat::BC4_R_UNORM;
        case CMP_FORMAT_BC5:
        case CMP_FORMAT_ATI2N_XY: return ImageFormat::BC5_RG_UNORM;
        case CMP_FORMAT_BC6H: return ImageFormat::BC6H_RGB_UFLOAT;
        case CMP_FORMAT_BC6H_SF: return ImageFormat::BC6H_RGB_SFLOAT;
        case CMP_FORMAT_BC7: return ImageFormat::BC7_RGBA_UNORM;
        default: return ImageFormat::UNDEFINED;
        }
    }

    std::string CMPErrorToString(int error) {
        switch (error) {
        case CMP_OK:                               return "Ok.";
        case CMP_ABORTED:                          return "The conversion was aborted.";
        case CMP_ERR_INVALID_SOURCE_TEXTURE:       return "The source texture is invalid.";
        case CMP_ERR_INVALID_DEST_TEXTURE:         return "The destination texture is invalid.";
        case CMP_ERR_UNSUPPORTED_SOURCE_FORMAT:    return "The source format is not a supported format.";
        case CMP_ERR_UNSUPPORTED_DEST_FORMAT:      return "The destination format is not a supported format.";
        case CMP_ERR_UNSUPPORTED_GPU_ASTC_DECODE:  return "The GPU hardware is not supported for ASTC decoding.";
        case CMP_ERR_UNSUPPORTED_GPU_BASIS_DECODE: return "The GPU hardware is not supported for BASIS decoding.";
        case CMP_ERR_SIZE_MISMATCH:                return "The source and destination texture sizes do not match.";
        case CMP_ERR_UNABLE_TO_INIT_CODEC:         return "Compressonator was unable to initialize the codec needed for conversion.";
        case CMP_ERR_UNABLE_TO_INIT_DECOMPRESSLIB: return "GPU_Decode Lib was unable to initialize the codec needed for decompression.";
        case CMP_ERR_UNABLE_TO_INIT_COMPUTELIB:    return "Compute Lib was unable to initialize the codec needed for compression.";
        case CMP_ERR_CMP_DESTINATION:              return "Error in compressing destination texture.";
        case CMP_ERR_MEM_ALLOC_FOR_MIPSET:         return "Memory error: allocating MIPSet compression level data buffer.";
        case CMP_ERR_UNKNOWN_DESTINATION_FORMAT:   return "The destination codec type is unknown.";
        case CMP_ERR_FAILED_HOST_SETUP:            return "Failed to setup host for processing.";
        case CMP_ERR_PLUGIN_FILE_NOT_FOUND:        return "The required plugin library was not found.";
        case CMP_ERR_UNABLE_TO_LOAD_FILE:          return "The requested file was not loaded.";
        case CMP_ERR_UNABLE_TO_CREATE_ENCODER:     return "Request to create an encoder failed.";
        case CMP_ERR_UNABLE_TO_LOAD_ENCODER:       return "Unable to load an encoder library.";
        case CMP_ERR_NOSHADER_CODE_DEFINED:        return "No shader code is available for the requested framework.";
        case CMP_ERR_GPU_DOESNOT_SUPPORT_COMPUTE:  return "The GPU device selected does not support compute.";
        case CMP_ERR_NOPERFSTATS:                  return "No performance stats are available.";
        case CMP_ERR_GPU_DOESNOT_SUPPORT_CMP_EXT:  return "The GPU does not support the requested compression extension.";
        case CMP_ERR_GAMMA_OUTOFRANGE:             return "Gamma value set for processing is out of range.";
        case CMP_ERR_PLUGIN_SHAREDIO_NOT_SET:      return "The plugin shared IO call was not set and is required for this plugin to operate.";
        case CMP_ERR_UNABLE_TO_INIT_D3DX:          return "Unable to initialize DirectX SDK or get a specific DX API.";
        case CMP_FRAMEWORK_NOT_INITIALIZED:        return "CMP_InitFramework failed or not called.";
        case CMP_ERR_GENERIC:                      return "An unknown error occurred.";
        default:                                   return "Unknown CMP_ERROR value.";
        }
    }

    int GetChannelCountFromCMPFormat(int format) {
        switch (format) {
            // Uncompressed formats
        case CMP_FORMAT_RGBA_8888:
        case CMP_FORMAT_BGRA_8888:
        case CMP_FORMAT_ARGB_8888:
        case CMP_FORMAT_ABGR_8888:
        case CMP_FORMAT_RGBA_1010102:
        case CMP_FORMAT_ARGB_2101010:
            return 4; // 4 channels
        case CMP_FORMAT_RGB_888:
        case CMP_FORMAT_BGR_888:
        case CMP_FORMAT_RGBE_32F:
            return 3; // 3 channels
        case CMP_FORMAT_RG_8:
        case CMP_FORMAT_RG_16:
        case CMP_FORMAT_RG_16F:
        case CMP_FORMAT_RG_32F:
            return 2; // 2 channels
        case CMP_FORMAT_R_8:
        case CMP_FORMAT_R_16:
        case CMP_FORMAT_R_16F:
        case CMP_FORMAT_R_32F:
            return 1; // 1 channel
            // Compressed formats
        case CMP_FORMAT_BC1:    // DXT1
            return 3; // RGB
        case CMP_FORMAT_BC2:    // DXT3
        case CMP_FORMAT_BC3:    // DXT5
            return 4; // RGBA
        case CMP_FORMAT_BC4:    // ATI1
            return 1; // Red only
        case CMP_FORMAT_BC5:    // ATI2
            return 2; // Red, Green
        case CMP_FORMAT_BC6H:   // HDR (RGB Half)
            return 3; // RGB
        case CMP_FORMAT_BC7:    // RGB or RGBA
            return 4; // RGBA
        case CMP_FORMAT_ASTC:   // Adaptive Scalable Texture Compression
            return 4; // Typically RGBA
        case CMP_FORMAT_ETC_RGB:
            return 3; // RGB
        case CMP_FORMAT_ETC2_RGBA:
            return 4; // RGBA
        default:
            return 0; // Unknown or unsupported format
        }
    }

    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool generateMipMaps) {
        if (!IsCMPFrameworkInitialized()) {
            InitializeCMPFramework();
        }
        CMP_MipSet mipSetIn = {};
        CMP_MipSet mipSetOut = {};
        KernelOptions kernelOptions = {};
        CMP_ERROR status;

        // Load the texture
        status = CMP_LoadTexture(inputFilepath.c_str(), &mipSetIn);
        if (status != CMP_OK) {
            std::cout << "Error: Failed to load texture. Error code: " << CMPErrorToString(status) << "\n";
            return;
        }
        // Generate mipmaps
        if (generateMipMaps) {
            CMP_INT mipmapLevelCount = (CMP_INT)(std::log2(std::max(mipSetIn.m_nWidth, mipSetIn.m_nHeight))) + 1;
            CMP_INT minSize = CMP_CalcMinMipSize(mipSetIn.m_nHeight, mipSetIn.m_nWidth, mipmapLevelCount);
            CMP_GenerateMIPLevels(&mipSetIn, minSize);
        }
        // Compression settings
        kernelOptions.encodeWith = CMP_HPC; // CMP_CPU // CMP_GPU_OCL
        kernelOptions.format = CMP_FORMAT_BC7;
        kernelOptions.fquality = 0.88;
        kernelOptions.threads = 0;

        memset(&mipSetOut, 0, sizeof(CMP_MipSet));
        status = CMP_ProcessTexture(&mipSetIn, &mipSetOut, kernelOptions, CompressionCallback);
        std::cout << "\n";
        if (status != CMP_OK) {
            std::cout << "Failed to process texture " << inputFilepath << ": " << CMPErrorToString(status) << "\n";
            return;
        }
        status = CMP_SaveTexture(outputFilepath.c_str(), &mipSetOut);
        if (status != CMP_OK) {
            CMP_FreeMipSet(&mipSetIn);
            std::cout << "Failed to save texture " << inputFilepath << ": " << CMPErrorToString(status) << "\n";
            return;
        }
        // Cleanup
        CMP_FreeMipSet(&mipSetIn);
        CMP_FreeMipSet(&mipSetOut);
    }

    std::string CMPFormatToString(int format) {
        switch (format) {
        case 0x0000: return "CMP_FORMAT_Unknown";
        case 0x0010: return "CMP_FORMAT_RGBA_8888_S";
        case 0x0020: return "CMP_FORMAT_ARGB_8888_S";
        case 0x0030: return "CMP_FORMAT_ARGB_8888";
        case 0x0040: return "CMP_FORMAT_ABGR_8888";
        case 0x0050: return "CMP_FORMAT_RGBA_8888";
        case 0x0060: return "CMP_FORMAT_BGRA_8888";
        case 0x0070: return "CMP_FORMAT_RGB_888";
        case 0x0080: return "CMP_FORMAT_RGB_888_S";
        case 0x0090: return "CMP_FORMAT_BGR_888";
        case 0x00A0: return "CMP_FORMAT_RG_8_S";
        case 0x00B0: return "CMP_FORMAT_RG_8";
        case 0x00C0: return "CMP_FORMAT_R_8_S";
        case 0x00D0: return "CMP_FORMAT_R_8";
        case 0x00E0: return "CMP_FORMAT_ARGB_2101010";
        case 0x00F0: return "CMP_FORMAT_RGBA_1010102";
        case 0x0100: return "CMP_FORMAT_ARGB_16";
        case 0x0110: return "CMP_FORMAT_ABGR_16";
        case 0x0120: return "CMP_FORMAT_RGBA_16";
        case 0x0130: return "CMP_FORMAT_BGRA_16";
        case 0x0140: return "CMP_FORMAT_RG_16";
        case 0x0150: return "CMP_FORMAT_R_16";
        case 0x1000: return "CMP_FORMAT_RGBE_32F";
        case 0x1010: return "CMP_FORMAT_ARGB_16F";
        case 0x1020: return "CMP_FORMAT_ABGR_16F";
        case 0x1030: return "CMP_FORMAT_RGBA_16F";
        case 0x1040: return "CMP_FORMAT_BGRA_16F";
        case 0x1050: return "CMP_FORMAT_RG_16F";
        case 0x1060: return "CMP_FORMAT_R_16F";
        case 0x1070: return "CMP_FORMAT_ARGB_32F";
        case 0x1080: return "CMP_FORMAT_ABGR_32F";
        case 0x1090: return "CMP_FORMAT_RGBA_32F";
        case 0x10A0: return "CMP_FORMAT_BGRA_32F";
        case 0x10B0: return "CMP_FORMAT_RGB_32F";
        case 0x10C0: return "CMP_FORMAT_BGR_32F";
        case 0x10D0: return "CMP_FORMAT_RG_32F";
        case 0x10E0: return "CMP_FORMAT_R_32F";
        case 0x2000: return "CMP_FORMAT_BROTLIG";
        case 0x0011: return "CMP_FORMAT_BC1";
        case 0x0021: return "CMP_FORMAT_BC2";
        case 0x0031: return "CMP_FORMAT_BC3";
        case 0x0041: return "CMP_FORMAT_BC4";
        case 0x1041: return "CMP_FORMAT_BC4_S";
        case 0x0051: return "CMP_FORMAT_BC5";
        case 0x1051: return "CMP_FORMAT_BC5_S";
        case 0x0061: return "CMP_FORMAT_BC6H";
        case 0x1061: return "CMP_FORMAT_BC6H_SF";
        case 0x0071: return "CMP_FORMAT_BC7";
        case 0x0141: return "CMP_FORMAT_ATI1N";
        case 0x0151: return "CMP_FORMAT_ATI2N";
        case 0x0152: return "CMP_FORMAT_ATI2N_XY";
        case 0x0153: return "CMP_FORMAT_ATI2N_DXT5";
        case 0x0211: return "CMP_FORMAT_DXT1";
        case 0x0221: return "CMP_FORMAT_DXT3";
        case 0x0231: return "CMP_FORMAT_DXT5";
        case 0x0252: return "CMP_FORMAT_DXT5_xGBR";
        case 0x0253: return "CMP_FORMAT_DXT5_RxBG";
        case 0x0254: return "CMP_FORMAT_DXT5_RBxG";
        case 0x0255: return "CMP_FORMAT_DXT5_xRBG";
        case 0x0256: return "CMP_FORMAT_DXT5_RGxB";
        case 0x0257: return "CMP_FORMAT_DXT5_xGxR";
        case 0x0301: return "CMP_FORMAT_ATC_RGB";
        case 0x0302: return "CMP_FORMAT_ATC_RGBA_Explicit";
        case 0x0303: return "CMP_FORMAT_ATC_RGBA_Interpolated";
        case 0x0A01: return "CMP_FORMAT_ASTC";
        case 0x0A02: return "CMP_FORMAT_APC";
        case 0x0A03: return "CMP_FORMAT_PVRTC";
        case 0x0E01: return "CMP_FORMAT_ETC_RGB";
        case 0x0E02: return "CMP_FORMAT_ETC2_RGB";
        case 0x0E03: return "CMP_FORMAT_ETC2_SRGB";
        case 0x0E04: return "CMP_FORMAT_ETC2_RGBA";
        case 0x0E05: return "CMP_FORMAT_ETC2_RGBA1";
        case 0x0E06: return "CMP_FORMAT_ETC2_SRGBA";
        case 0x0E07: return "CMP_FORMAT_ETC2_SRGBA1";
        case 0x0B01: return "CMP_FORMAT_BINARY";
        case 0x0B02: return "CMP_FORMAT_GTC";
        case 0x0B03: return "CMP_FORMAT_BASIS";
        case 0xFFFF: return "CMP_FORMAT_MAX";
        default: return "Unknown Format";
        }
    }

    void PrintMipSetInfo(CMP_MipSet& mipset) {
        std::cout << "Width: " << mipset.m_nWidth << "\n";
        std::cout << "Height: " << mipset.m_nHeight << "\n";
        std::cout << "Channels: " << std::to_string(mipset.m_nChannels) << "\n";
        std::cout << "Format: " << CMPFormatToString(mipset.m_format) << "\n";
        std::cout << "Mipmaps: " << mipset.m_nMaxMipLevels << "\n";
    }
}