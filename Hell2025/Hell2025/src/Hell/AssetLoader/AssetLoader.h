#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/File.h"

#include <string>
#include <vector>

namespace Hell::AssetLoader {

    void Init();
    void DiscoverAssets();
    void Update();
    void LoadMinimumRequiredAssets(); // TODO: rename to something that doesn't fell like a boolean
    bool LoadingComplete();
    void OnLoadingComplete();
    void AddLoadLogItem(std::string text);
    std::vector<std::string>& GetLoadLog();

    AnimationData LoadAnimationData(FileInfo fileInfo);
    void CreateSpriteSheets();
    void LoadIESFiles();
    void LoadMidiFiles();
    void LoadSoundFonts();
}
