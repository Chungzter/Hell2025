#include "EditorLayout.h"

#include "EditorCoordinates.h"
#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace Unloved::EditorSession::Layout {
    namespace {
        constexpr int32_t DEFAULT_FILE_MENU_HEIGHT = 26;
        constexpr int32_t DEFAULT_PANEL_WIDTH = 320;
        constexpr int32_t PANEL_HEADING_HEIGHT = 26;
        constexpr int32_t MIN_VIEWPORTS_WIDTH = 200;
        constexpr int32_t MIN_SIDE_PANEL_WIDTH = 160;
        constexpr int32_t MIN_VIEWPORT_REGION_SIZE = 96;
        constexpr int32_t DIVIDER_HIT_RADIUS = 6;

        const glm::vec4 FILE_MENU_COLOR = glm::vec4(0.101961f, 0.090196f, 0.121569f, 1.0f);    // #1a171f
        const glm::vec4 PANEL_COLOR = glm::vec4(0.058824f, 0.050980f, 0.070588f, 1.0f);        // #0f0d12
        const glm::vec4 PANEL_HEADING_COLOR = glm::vec4(0.101961f, 0.090196f, 0.121569f, 1.0f); // #1a171f
        const glm::vec4 BORDER_COLOR = glm::vec4(0.42f, 0.40f, 0.46f, 1.0f);

        int32_t g_fileMenuHeight = DEFAULT_FILE_MENU_HEIGHT;
        int32_t g_hierarchyWidth = DEFAULT_PANEL_WIDTH;
        int32_t g_propertiesWidth = DEFAULT_PANEL_WIDTH;
        int32_t g_layoutWidth = 1;
        int32_t g_layoutHeight = 1;
        float g_viewportSplitX = 0.5f;
        float g_viewportSplitY = 0.5f;
        EditorViewportLayout g_viewportLayout = EditorViewportLayout::SINGLE;

        enum class Divider {
            NONE,
            HIERARCHY_RIGHT,
            PROPERTIES_LEFT,
            VIEWPORT_VERTICAL,
            VIEWPORT_HORIZONTAL,
            VIEWPORT_BOTH
        };

        Divider g_hoveredDivider = Divider::NONE;
        Divider g_activeDivider = Divider::NONE;
        glm::ivec2 g_dividerGrabOffset = glm::ivec2(0);
        bool g_dividerCursorActive = false;

        EditorPanel CreatePanel(EditorPanelEdge edges, const glm::vec4& backgroundColor, bool drawBackground = true) {
            EditorPanel panel;
            panel.edges = edges;
            panel.backgroundColor = backgroundColor;
            panel.borderColor = BORDER_COLOR;
            panel.borderThickness = 1;
            panel.drawBackground = drawBackground;
            return panel;
        }

        EditorPanel g_fileMenuPanel = CreatePanel(EditorPanelEdge::BOTTOM, FILE_MENU_COLOR);
        EditorPanel g_hierarchyPanel = CreatePanel(EditorPanelEdge::RIGHT, PANEL_COLOR);
        EditorPanel g_viewportsPanel = CreatePanel(EditorPanelEdge::NONE, glm::vec4(0.0f), false);
        EditorPanel g_propertiesPanel = CreatePanel(EditorPanelEdge::LEFT, PANEL_COLOR);
        std::array<EditorViewportRegion, 4> g_viewportRegions;

        int32_t GetViewportSplitOffset(int32_t extent, float split) {
            if (extent <= 0) return 0;
            const int32_t minimumSize = std::min(MIN_VIEWPORT_REGION_SIZE, extent / 2);
            return std::clamp(static_cast<int32_t>(std::round(static_cast<float>(extent) * split)), minimumSize, extent - minimumSize);
        }

        int32_t GetRequiredViewportWidth() {
            const int32_t columnCount = g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR ? 2 : 1;
            return std::max(MIN_VIEWPORTS_WIDTH, columnCount * MIN_VIEWPORT_REGION_SIZE);
        }

        void SetViewportRegion(uint32_t index, const EditorRect& rect) {
            EditorViewportRegion& region = g_viewportRegions[index];
            region.rect = rect;
            region.visible = rect.HasArea();

            const float inverseWidth = 1.0f / static_cast<float>(g_layoutWidth);
            const float inverseHeight = 1.0f / static_cast<float>(g_layoutHeight);
            region.normalizedPosition.x = static_cast<float>(rect.x) * inverseWidth;
            region.normalizedPosition.y = static_cast<float>(rect.y) * inverseHeight;
            region.normalizedSize.x = static_cast<float>(rect.width) * inverseWidth;
            region.normalizedSize.y = static_cast<float>(rect.height) * inverseHeight;
        }

        void UpdateViewportRegions() {
            for (EditorViewportRegion& region : g_viewportRegions) {
                region = {};
            }

            const EditorRect& rect = g_viewportsPanel.rect;
            if (g_viewportLayout == EditorViewportLayout::SINGLE) {
                SetViewportRegion(0, rect);
                return;
            }

            const int32_t leftWidth = GetViewportSplitOffset(rect.width, g_viewportSplitX);
            const int32_t rightWidth = rect.width - leftWidth;

            if (g_viewportLayout == EditorViewportLayout::LEFT_RIGHT) {
                SetViewportRegion(0, { rect.x, rect.y, leftWidth, rect.height });
                SetViewportRegion(1, { rect.x + leftWidth, rect.y, rightWidth, rect.height });
                return;
            }

            const int32_t topHeight = GetViewportSplitOffset(rect.height, g_viewportSplitY);
            const int32_t bottomHeight = rect.height - topHeight;
            if (g_viewportLayout == EditorViewportLayout::TOP_BOTTOM) {
                SetViewportRegion(0, { rect.x, rect.y, rect.width, topHeight });
                SetViewportRegion(1, { rect.x, rect.y + topHeight, rect.width, bottomHeight });
                return;
            }

            SetViewportRegion(0, { rect.x, rect.y, leftWidth, topHeight });
            SetViewportRegion(1, { rect.x + leftWidth, rect.y, rightWidth, topHeight });
            SetViewportRegion(2, { rect.x, rect.y + topHeight, leftWidth, bottomHeight });
            SetViewportRegion(3, { rect.x + leftWidth, rect.y + topHeight, rightWidth, bottomHeight });
        }

        EditorPanel* GetPanelById(EditorPanelId panelId) {
            switch (panelId) {
                case EditorPanelId::FILE_MENU:  return &g_fileMenuPanel;
                case EditorPanelId::HIERARCHY:  return &g_hierarchyPanel;
                case EditorPanelId::VIEWPORTS:  return &g_viewportsPanel;
                case EditorPanelId::PROPERTIES: return &g_propertiesPanel;
            }
            return nullptr;
        }

        EditorRect GetPanelHeadingRect(const EditorPanel& panel) {
            return { panel.rect.x, panel.rect.y, panel.rect.width, std::min(PANEL_HEADING_HEIGHT, panel.rect.height) };
        }

        EditorRect GetPanelContentRect(const EditorPanel& panel) {
            const int32_t headingHeight = std::min(PANEL_HEADING_HEIGHT, panel.rect.height);
            return { panel.rect.x, panel.rect.y + headingHeight, panel.rect.width, panel.rect.height - headingHeight };
        }

        void DrawPanelHeadingBackground(const EditorPanel& panel) {
            if (!panel.visible || !panel.rect.HasArea()) return;
            UI::DrawSolidRect(GetPanelHeadingRect(panel), PANEL_HEADING_COLOR);
        }

        void DrawPanelHeadingText(const EditorPanel& panel, const char* heading) {
            if (!panel.visible || !panel.rect.HasArea()) return;

            const EditorRect headingRect = GetPanelHeadingRect(panel);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + heading, Style::FONT_NAME, glm::ivec2(headingRect.x + 10, headingRect.y + headingRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);
        }

        bool IsNearVerticalDivider(const glm::ivec2& mousePosition, int32_t dividerX, const EditorRect& bounds) {
            return bounds.HasArea() && mousePosition.y >= bounds.y && mousePosition.y < bounds.Bottom() && std::abs(mousePosition.x - dividerX) <= DIVIDER_HIT_RADIUS;
        }

        bool IsNearHorizontalDivider(const glm::ivec2& mousePosition, int32_t dividerY, const EditorRect& bounds) {
            return bounds.HasArea() && mousePosition.x >= bounds.x && mousePosition.x < bounds.Right() && std::abs(mousePosition.y - dividerY) <= DIVIDER_HIT_RADIUS;
        }

        Divider FindHoveredDivider(const glm::ivec2& mousePosition) {
            if (IsNearVerticalDivider(mousePosition, g_hierarchyPanel.rect.Right(), g_hierarchyPanel.rect)) return Divider::HIERARCHY_RIGHT;
            if (IsNearVerticalDivider(mousePosition, g_propertiesPanel.rect.x, g_propertiesPanel.rect)) return Divider::PROPERTIES_LEFT;

            const bool verticalSplitVisible = g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR;
            const bool horizontalSplitVisible = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM || g_viewportLayout == EditorViewportLayout::FOUR;
            const int32_t horizontalSplitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
            const bool verticalSplitHovered = verticalSplitVisible && IsNearVerticalDivider(mousePosition, g_viewportRegions[1].rect.x, g_viewportsPanel.rect);
            const bool horizontalSplitHovered = horizontalSplitVisible && IsNearHorizontalDivider(mousePosition, horizontalSplitY, g_viewportsPanel.rect);

            if (verticalSplitHovered && horizontalSplitHovered) return Divider::VIEWPORT_BOTH;
            if (verticalSplitHovered) return Divider::VIEWPORT_VERTICAL;
            if (horizontalSplitHovered) return Divider::VIEWPORT_HORIZONTAL;
            return Divider::NONE;
        }

        void ResizeHierarchyPanel(int32_t mouseX) {
            const int32_t maximumWidth = std::max(0, g_layoutWidth - g_propertiesPanel.rect.width - GetRequiredViewportWidth());
            const int32_t minimumWidth = std::min(MIN_SIDE_PANEL_WIDTH, maximumWidth);
            g_hierarchyWidth = std::clamp(mouseX, minimumWidth, maximumWidth);
        }

        void ResizePropertiesPanel(int32_t mouseX) {
            const int32_t maximumWidth = std::max(0, g_layoutWidth - g_hierarchyPanel.rect.width - GetRequiredViewportWidth());
            const int32_t minimumWidth = std::min(MIN_SIDE_PANEL_WIDTH, maximumWidth);
            g_propertiesWidth = std::clamp(g_layoutWidth - mouseX, minimumWidth, maximumWidth);
        }

        void ResizeViewportVerticalSplit(int32_t mouseX) {
            if (g_viewportsPanel.rect.width <= 0) return;
            const int32_t minimumSize = std::min(MIN_VIEWPORT_REGION_SIZE, g_viewportsPanel.rect.width / 2);
            const int32_t splitOffset = std::clamp(mouseX - g_viewportsPanel.rect.x, minimumSize, g_viewportsPanel.rect.width - minimumSize);
            g_viewportSplitX = static_cast<float>(splitOffset) / static_cast<float>(g_viewportsPanel.rect.width);
        }

        void ResizeViewportHorizontalSplit(int32_t mouseY) {
            if (g_viewportsPanel.rect.height <= 0) return;
            const int32_t minimumSize = std::min(MIN_VIEWPORT_REGION_SIZE, g_viewportsPanel.rect.height / 2);
            const int32_t splitOffset = std::clamp(mouseY - g_viewportsPanel.rect.y, minimumSize, g_viewportsPanel.rect.height - minimumSize);
            g_viewportSplitY = static_cast<float>(splitOffset) / static_cast<float>(g_viewportsPanel.rect.height);
        }

        void ResizeActiveDivider(const glm::ivec2& mousePosition) {
            const glm::ivec2 dividerPosition = mousePosition - g_dividerGrabOffset;
            switch (g_activeDivider) {
                case Divider::HIERARCHY_RIGHT:     ResizeHierarchyPanel(dividerPosition.x);       break;
                case Divider::PROPERTIES_LEFT:     ResizePropertiesPanel(dividerPosition.x);      break;
                case Divider::VIEWPORT_VERTICAL:   ResizeViewportVerticalSplit(dividerPosition.x); break;
                case Divider::VIEWPORT_HORIZONTAL: ResizeViewportHorizontalSplit(dividerPosition.y); break;
                case Divider::VIEWPORT_BOTH:       ResizeViewportVerticalSplit(dividerPosition.x); ResizeViewportHorizontalSplit(dividerPosition.y); break;
                default: break;
            }
        }

        void BeginDividerDrag(const glm::ivec2& mousePosition) {
            g_activeDivider = g_hoveredDivider;
            g_dividerGrabOffset = glm::ivec2(0);

            if (g_activeDivider == Divider::HIERARCHY_RIGHT || g_activeDivider == Divider::PROPERTIES_LEFT) {
                g_hierarchyWidth = g_hierarchyPanel.rect.width;
                g_propertiesWidth = g_propertiesPanel.rect.width;
            }

            if (g_activeDivider == Divider::HIERARCHY_RIGHT) g_dividerGrabOffset.x = mousePosition.x - g_hierarchyPanel.rect.Right();
            else if (g_activeDivider == Divider::PROPERTIES_LEFT) g_dividerGrabOffset.x = mousePosition.x - g_propertiesPanel.rect.x;
            else if (g_activeDivider == Divider::VIEWPORT_VERTICAL) g_dividerGrabOffset.x = mousePosition.x - g_viewportRegions[1].rect.x;
            else if (g_activeDivider == Divider::VIEWPORT_HORIZONTAL) {
                const int32_t splitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
                g_dividerGrabOffset.y = mousePosition.y - splitY;
            }
            else if (g_activeDivider == Divider::VIEWPORT_BOTH) g_dividerGrabOffset = mousePosition - glm::ivec2(g_viewportRegions[1].rect.x, g_viewportRegions[2].rect.y);
        }

        void ApplyDividerCursor() {
            const Divider divider = g_activeDivider != Divider::NONE ? g_activeDivider : g_hoveredDivider;
            if (divider == Divider::NONE) {
                if (g_dividerCursorActive) Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
                g_dividerCursorActive = false;
                return;
            }

            if (divider == Divider::VIEWPORT_BOTH) Hell::BackEnd::SetCursor(HELL_CURSOR_CROSSHAIR);
            else if (divider == Divider::VIEWPORT_HORIZONTAL) Hell::BackEnd::SetCursor(HELL_CURSOR_VRESIZE);
            else Hell::BackEnd::SetCursor(HELL_CURSOR_HRESIZE);
            g_dividerCursorActive = true;
        }

    }

    void Update() {
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        g_layoutWidth = std::max(1, static_cast<int32_t>(resolution.x));
        g_layoutHeight = std::max(1, static_cast<int32_t>(resolution.y));

        const int32_t fileMenuHeight = std::clamp(g_fileMenuHeight, 0, g_layoutHeight);
        const int32_t availableSideWidth = std::max(0, g_layoutWidth - GetRequiredViewportWidth());
        const int32_t requestedSideWidth = std::max(0, g_hierarchyWidth) + std::max(0, g_propertiesWidth);
        const float sideScale = requestedSideWidth > availableSideWidth && requestedSideWidth > 0
            ? static_cast<float>(availableSideWidth) / static_cast<float>(requestedSideWidth)
            : 1.0f;
        const int32_t hierarchyWidth = static_cast<int32_t>(std::round(static_cast<float>(std::max(0, g_hierarchyWidth)) * sideScale));
        const int32_t propertiesWidth = std::min(static_cast<int32_t>(std::round(static_cast<float>(std::max(0, g_propertiesWidth)) * sideScale)), g_layoutWidth - hierarchyWidth);
        const int32_t viewportWidth = std::max(0, g_layoutWidth - hierarchyWidth - propertiesWidth);
        const int32_t workspaceHeight = g_layoutHeight - fileMenuHeight;

        g_fileMenuPanel.rect = { 0, 0, g_layoutWidth, fileMenuHeight };
        g_hierarchyPanel.rect = { 0, fileMenuHeight, hierarchyWidth, workspaceHeight };
        g_viewportsPanel.rect = { hierarchyWidth, fileMenuHeight, viewportWidth, workspaceHeight };
        g_propertiesPanel.rect = { hierarchyWidth + viewportWidth, fileMenuHeight, propertiesWidth, workspaceHeight };

        UpdateViewportRegions();
    }

    void UpdateDividerInput(bool allowInput) {
        if (!Hell::BackEnd::WindowHasFocus()) {
            CancelInteraction();
            return;
        }

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();

        if (!Hell::Input::LeftMouseDown()) {
            g_activeDivider = Divider::NONE;
        }

        g_hoveredDivider = g_activeDivider == Divider::NONE && allowInput ? FindHoveredDivider(mousePosition) : g_activeDivider;
        if (allowInput && Hell::Input::LeftMousePressed() && g_hoveredDivider != Divider::NONE) {
            BeginDividerDrag(mousePosition);
        }

        if (g_activeDivider != Divider::NONE && Hell::Input::LeftMouseDown()) {
            ResizeActiveDivider(mousePosition);
            Update();
        }

        ApplyDividerCursor();
    }

    void RenderBackgrounds() {
        UI::DrawPanelBackground(g_fileMenuPanel);
        UI::DrawPanelBackground(g_hierarchyPanel);
        UI::DrawPanelBackground(g_viewportsPanel);
        UI::DrawPanelBackground(g_propertiesPanel);

        DrawPanelHeadingBackground(g_hierarchyPanel);
        DrawPanelHeadingBackground(g_propertiesPanel);
    }

    void RenderOverlay() {
        DrawPanelHeadingText(g_hierarchyPanel, "Hierarchy");
        DrawPanelHeadingText(g_propertiesPanel, "Properties");

        UI::DrawPanelEdges(g_fileMenuPanel);
        UI::DrawPanelEdges(g_hierarchyPanel);
        UI::DrawPanelEdges(g_viewportsPanel);
        UI::DrawPanelEdges(g_propertiesPanel);

        if (g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR) {
            const int32_t splitX = g_viewportRegions[1].rect.x;
            UI::DrawLine(glm::vec2(splitX, g_viewportsPanel.rect.y), glm::vec2(splitX, g_viewportsPanel.rect.Bottom()), BORDER_COLOR);
        }
        if (g_viewportLayout == EditorViewportLayout::TOP_BOTTOM || g_viewportLayout == EditorViewportLayout::FOUR) {
            const int32_t splitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
            UI::DrawLine(glm::vec2(g_viewportsPanel.rect.x, splitY), glm::vec2(g_viewportsPanel.rect.Right(), splitY), BORDER_COLOR);
        }
    }

    void SetFileMenuHeight(int32_t height) {
        g_fileMenuHeight = std::max(0, height);
    }

    void SetHierarchyWidth(int32_t width) {
        g_hierarchyWidth = std::max(0, width);
    }

    void SetPropertiesWidth(int32_t width) {
        g_propertiesWidth = std::max(0, width);
    }

    void SetViewportLayout(EditorViewportLayout layout) {
        g_viewportLayout = layout;
        CancelInteraction();
    }

    void CancelInteraction() {
        g_hoveredDivider = Divider::NONE;
        g_activeDivider = Divider::NONE;
        g_dividerGrabOffset = glm::ivec2(0);
        if (g_dividerCursorActive) Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        g_dividerCursorActive = false;
    }

    void SetPanelEdges(EditorPanelId panelId, EditorPanelEdge edges) {
        if (EditorPanel* panel = GetPanelById(panelId)) {
            panel->edges = edges;
        }
    }

    int32_t GetFileMenuHeight() {
        return g_fileMenuHeight;
    }

    int32_t GetHierarchyWidth() {
        return g_hierarchyWidth;
    }

    int32_t GetPropertiesWidth() {
        return g_propertiesWidth;
    }

    uint32_t GetViewportCount() {
        if (g_viewportLayout == EditorViewportLayout::SINGLE) return 1;
        if (g_viewportLayout == EditorViewportLayout::FOUR) return 4;
        return 2;
    }

    EditorViewportLayout GetViewportLayout() {
        return g_viewportLayout;
    }

    bool WantsMouseCapture() {
        return g_hoveredDivider != Divider::NONE || g_activeDivider != Divider::NONE;
    }

    const EditorPanel& GetFileMenuPanel() {
        return g_fileMenuPanel;
    }

    const EditorPanel& GetHierarchyPanel() {
        return g_hierarchyPanel;
    }

    const EditorPanel& GetViewportsPanel() {
        return g_viewportsPanel;
    }

    const EditorPanel& GetPropertiesPanel() {
        return g_propertiesPanel;
    }

    EditorRect GetHierarchyContentRect() {
        return GetPanelContentRect(g_hierarchyPanel);
    }

    EditorRect GetPropertiesContentRect() {
        return GetPanelContentRect(g_propertiesPanel);
    }

    const EditorViewportRegion* GetViewportRegionByIndex(uint32_t index) {
        return index < g_viewportRegions.size() ? &g_viewportRegions[index] : nullptr;
    }
}
