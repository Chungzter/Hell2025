#pragma once

#include "Hell/Render/TextureTypes.h"

#include <string>

struct IESProfile;

namespace Hell::AssetLoader {

    void DiscoverAssets();
    void Update();
    void LoadRequired();
    void LoadIESFiles();
    bool LoadingComplete();

    bool LoadIES(const std::string& path, IESProfile& outProfile);
}
