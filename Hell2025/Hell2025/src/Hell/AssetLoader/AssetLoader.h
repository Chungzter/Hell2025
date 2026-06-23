#pragma once

#include "Hell/Render/TextureTypes.h"

#include <string>

struct IESProfile;

namespace Hell::AssetLoader {

    void DiscoverAssets();
    void Update();
    void LoadFonts();
    void LoadIESFiles();
    bool LoadingComplete();

    ImageData LoadImageData(const std::string& path, ImageDataType type);
    bool LoadIES(const std::string& path, IESProfile& outProfile);
}
