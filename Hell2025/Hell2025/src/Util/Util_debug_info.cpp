#include "Util.h"

namespace Util {

    void PrintDebugInfo(const ImageData& imageData) {
        std::cout << "Format: " << ImageFormatToString(imageData.format) << "\n";
        std::cout << "Image Data Type: " << Util::ImageDataTypeToString(imageData.type) << "\n";
        std::cout << "Channel Count: " << GetImageFormatChannelCount(imageData.format) << "\n";
        std::cout << "Mip Count: " << imageData.mips.size() << "\n";
        for (size_t i = 0; i < imageData.mips.size(); ++i) {
            const TextureMip& mip = imageData.mips[i];
            std::cout << "Mip " << i << ": " << mip.width << "x" << mip.height
                << ", " << mip.data.size() << " bytes\n";
        }
    }

    std::string BytesToMBString(size_t bytes) {
        double megabytes = bytes / (1024.0 * 1024.0);
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << megabytes << " MB";
        return stream.str();
    }
}
