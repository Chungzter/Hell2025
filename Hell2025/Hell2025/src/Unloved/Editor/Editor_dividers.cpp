#include "Editor.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/Math/Range.h"

#include "Unloved/Render/Renderer.h"

#include "Unloved/Config/Config.h"

namespace Input = Hell::Input;

namespace Unloved::Editor {

    bool g_horizontalDividerHovered = false;
    bool g_verticalDividerHovered = false;
    int g_dividerHoverThreshold = 10;

    void UpdateDividers() {
        if (Editor::GetEditorViewportSplitMode() == EditorViewportSplitMode::SINGLE) return;

        // const Resolutions& resolutions = Config::GetResolutions();
        int mouseX = Input::GetMouseX();
        int mouseY = Input::GetMouseY();
        int windowWidth = Hell::BackEnd::GetCurrentWindowWidth();
        int windowHeight = Hell::BackEnd::GetCurrentWindowHeight();
        //int gBufferWidth = resolutions.gBuffer.x;
        //int gBufferHeight = resolutions.gBuffer.y;

        // Hover
        g_horizontalDividerHovered = (std::abs(mouseX - (GetVerticalDividerXPos() * windowWidth)) < g_dividerHoverThreshold);
        g_verticalDividerHovered = (std::abs(mouseY - (GetHorizontalDividerYPos() * windowHeight)) < g_dividerHoverThreshold);

        // Start drag
        if (Input::LeftMousePressed()) {
            if (IsHorizontalDividerHovered() && IsVerticalDividerHovered()) {
                SetEditorState(EditorState::RESIZING_HORIZONTAL_VERTICAL);
            }
            else if (IsHorizontalDividerHovered()) {
                SetEditorState(EditorState::RESIZING_HORIZONTAL);
            }
            else if (IsVerticalDividerHovered()) {
                SetEditorState(EditorState::RESIZING_VERTICAL);
            }
        }
        // End drag
        if (!Input::LeftMouseDown()) {
            if (GetEditorState() == EditorState::RESIZING_HORIZONTAL_VERTICAL ||
                GetEditorState() == EditorState::RESIZING_HORIZONTAL ||
                GetEditorState() == EditorState::RESIZING_VERTICAL) {
                SetEditorState(EditorState::IDLE);
            }
        }

        // Update horizontal divider
        if (GetEditorState() == EditorState::RESIZING_HORIZONTAL ||
            GetEditorState() == EditorState::RESIZING_HORIZONTAL_VERTICAL) {
            float xPos = Hell::Math::MapRange(mouseX, 0.0f, windowWidth, 0.0f, 1.0f);
            xPos = glm::clamp(xPos, 0.0f, 1.0f);
            Editor::SetSplitX(xPos);
        }

        // Update vertical divider
        if (GetEditorState() == EditorState::RESIZING_VERTICAL ||
            GetEditorState() == EditorState::RESIZING_HORIZONTAL_VERTICAL) {
            float yPos = Hell::Math::MapRange(mouseY, 0.0f, windowHeight, 0.0f, 1.0f);
            yPos = glm::clamp(yPos, 0.0f, 1.0f);
            Editor::SetSplitY(yPos);
        }
    }

    bool IsHorizontalDividerHovered() {
        return g_horizontalDividerHovered;
    }

    bool IsVerticalDividerHovered() {
        return g_verticalDividerHovered;
    }
}
