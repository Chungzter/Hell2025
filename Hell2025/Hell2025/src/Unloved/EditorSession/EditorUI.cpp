#include "EditorUI.h"

#include "Hell/UI/UIBackEnd.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

namespace Unloved::EditorSession::UI {

    void DrawSolidRect(const EditorRect& rect, const glm::vec4& color) {
        if (!rect.HasArea() || color.a <= 0.0f) return;

        UIBackEnd::BlitTexture(UICanvas::NATIVE, "White", glm::ivec2(rect.x, rect.y), Alignment::TOP_LEFT, color, glm::ivec2(rect.width, rect.height));
    }

    void DrawLine(const glm::vec2& begin, const glm::vec2& end, const glm::vec4& color, float thickness) {
        const glm::vec2 delta = end - begin;
        const float length = glm::length(delta);
        if (length <= 0.0f || thickness <= 0.0f || color.a <= 0.0f) return;

        const glm::vec2 midpoint = (begin + end) * 0.5f;
        const glm::ivec2 location = glm::ivec2(static_cast<int32_t>(std::round(midpoint.x)), static_cast<int32_t>(std::round(midpoint.y)));
        const glm::ivec2 size = glm::ivec2(std::max(1, static_cast<int32_t>(std::round(length))), std::max(1, static_cast<int32_t>(std::round(thickness))));
        const float rotation = std::atan2(delta.y, delta.x);

        UIBackEnd::BlitTexture(UICanvas::NATIVE, "White", location, Alignment::CENTERED, color, size, TextureFilter::NEAREST, rotation);
    }

    void DrawPanelBackground(const EditorPanel& panel) {
        if (!panel.visible || !panel.drawBackground) return;
        DrawSolidRect(panel.rect, panel.backgroundColor);
    }

    void DrawPanelEdges(const EditorPanel& panel) {
        if (!panel.visible || !panel.rect.HasArea() || panel.borderThickness <= 0) return;

        const int32_t thickness = panel.borderThickness;
        if (HasPanelEdge(panel.edges, EditorPanelEdge::TOP)) {
            DrawSolidRect({ panel.rect.x, panel.rect.y, panel.rect.width, thickness }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::BOTTOM)) {
            DrawSolidRect({ panel.rect.x, panel.rect.Bottom() - thickness, panel.rect.width, thickness }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::LEFT)) {
            DrawSolidRect({ panel.rect.x, panel.rect.y, thickness, panel.rect.height }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::RIGHT)) {
            DrawSolidRect({ panel.rect.Right() - thickness, panel.rect.y, thickness, panel.rect.height }, panel.borderColor);
        }
    }

    void DrawPanel(const EditorPanel& panel) {
        DrawPanelBackground(panel);
        DrawPanelEdges(panel);
    }
}
