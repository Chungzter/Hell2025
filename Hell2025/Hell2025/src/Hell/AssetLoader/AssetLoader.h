#pragma once

namespace Hell::AssetLoader {

    void DiscoverAssets();
    void Update();
    void LoadMinimumRequiredAssets(); // TODO: rename to something that doesn't fell like a boolean
    bool LoadingComplete();

    void LoadAnimations();
    void LoadIESFiles();
}
