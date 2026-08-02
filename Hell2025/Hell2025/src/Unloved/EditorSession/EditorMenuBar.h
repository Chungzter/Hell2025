#pragma once

#include "PlacementTools.h"

#include <cstdint>

namespace Unloved::EditorSession::MenuBar {

    enum class EditorMenuAction : uint8_t {
        NONE,
        NEW_FILE,
        OPEN_FILE,
        SAVE,
        CLOSE_EDITOR,
        EXIT_APPLICATION,
        VIEWPORT_SINGLE,
        VIEWPORT_LEFT_RIGHT,
        VIEWPORT_TOP_BOTTOM,
        VIEWPORT_FOUR
    };

    void Init();
    void RefreshLayout();
    void Update();
    void Render();
    void Close();

    bool WantsMouseCapture();
    bool WantsKeyboardCapture();

    // EditorSession consumes all actions
    EditorMenuAction ConsumeAction();
    PlacementTool ConsumePlacementTool();
}
