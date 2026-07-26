#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession {

    struct EditorScrollBar {
        EditorRect trackRect;
        EditorRect thumbRect;
        int32_t value = 0;
        int32_t dragOffset = 0;
        bool hovered = false;
        bool dragging = false;
        bool visible = false;
    };
}

namespace Unloved::EditorSession::ScrollBar {

    void Update(EditorScrollBar& scrollBar, const EditorRect& rect, int32_t contentSize, int32_t visibleSize, bool allowInput);
    void Render(const EditorScrollBar& scrollBar);
    bool WantsMouseCapture(const EditorScrollBar& scrollBar);
}
