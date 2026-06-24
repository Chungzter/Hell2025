#pragma once
#include "Hell/Common.h"
#include <string>

namespace Hell::BackEnd {
    // Core
    bool Init(API api, WindowedMode windowMode, const std::string& title);
    void BeginFrame();
    void EndFrame();

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
