#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession::UI {

    void DrawSolidRect(const EditorRect& rect, const glm::vec4& color);
    void DrawLine(const glm::vec2& begin, const glm::vec2& end, const glm::vec4& color, float thickness = 1.0f);

    void DrawPanelBackground(const EditorPanel& panel);
    void DrawPanelEdges(const EditorPanel& panel);
    void DrawPanel(const EditorPanel& panel);
}
