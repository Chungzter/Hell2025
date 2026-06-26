#pragma once

namespace Unloved {
    bool Init();

    void BeginFrame();

    void UpdateLoadingScreen();
    void OnAssetLoadingComplete();

    void Update();
    void Render();

    void EndFrame();
    void CleanUp();
}
