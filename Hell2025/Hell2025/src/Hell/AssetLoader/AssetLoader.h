#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/File.h"

#include <cstdint>
#include <string>
#include <vector>

struct RagdollData;

namespace Hell::AssetLoader {

    void Init(uint32_t maxCompressedTextureResolution);
    void DiscoverAssets();
    void Update();
    void LoadMinimumRequiredAssets(); // TODO: rename to something that doesn't fell like a boolean
    bool LoadingComplete();
    void OnLoadingComplete();
    void AddLoadLogItem(std::string text);
    std::vector<std::string>& GetLoadLog();

    AnimationData LoadAnimationData(FileInfo fileInfo);
    RagdollData LoadRagdollData(const std::string& path);
    void CreateSpriteSheets();
    void LoadIESFiles();
    void LoadMidiFiles();
    void LoadPointAnimations();
    void LoadRagdollDataFiles();
    void LoadSoundFonts();
    void LoadVATFiles();
}
