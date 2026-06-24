#pragma once
#include "Hell/Common.h"

namespace BackEnd {
    // Core
    bool Init(API api, WindowedMode windowMode);
    void BeginFrame();
    void EndFrame();

    void UpdateLoadingScreen();
    void OnAssetLoadingComplete();
    void UpdateGame();
    void UpdateSubSystems();

    void CleanUp(); 

    void ToggleBindlessTextures();
    bool RenderDocFound();

    // API
    void SetAPI(API api);
    const API GetAPI();

    // Cursor
    void SetCursor(int cursor);

    // Window
    void* GetWindowPointer();
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ForceCloseWindow();
    bool WindowIsOpen();
    bool WindowHasFocus();
    bool WindowHasNotBeenForceClosed();
    bool WindowIsMinimized();
    int GetWindowedWidth();
    int GetWindowedHeight();
    int GetCurrentWindowWidth();
    int GetCurrentWindowHeight();
    int GetFullScreenWidth();
    int GetFullScreenHeight();
    const WindowedMode& GetWindowedMode();

    // Render Targets
    void SetPresentTargetSize(int width, int height);
    int GetPresentTargetWidth();
    int GetPresentTargetHeight();
}
