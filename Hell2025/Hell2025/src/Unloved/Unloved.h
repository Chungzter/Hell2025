#pragma once

namespace Unloved {
    bool Init();

    void BeginFrame();

    void OnAssetLoadingComplete();

    void UpdateLazyKeypresses();
    void UpdateLoadingScreen();

    void Update();
    void Render();

    void EndFrame();
    void CleanUp();
}
