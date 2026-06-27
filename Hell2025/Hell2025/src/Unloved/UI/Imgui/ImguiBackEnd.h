#pragma once

#include "Legacy/Game/Types.h"

#include "Unloved/UI/Imgui/Types/Types.h"

namespace ImGuiBackEnd {
    void Init();
    void Update();
    void RecordFileMenuHeight(uint32_t height);
    void BeginFrame();
    void UpdateInternalState();

    bool OwnsMouse();
    bool HasKeyboardFocus();
    uint32_t GetFileMenuHeight();
}
