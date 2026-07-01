#include "Editor.h"

#include "Hell/Backend/BackEnd.h"

#include "Unloved/Common/Constants.h"

namespace Unloved::Editor {

    void UpdateCursor() {
        // Resizing dividers
        if (IsHorizontalDividerHovered() && IsVerticalDividerHovered()) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_CROSSHAIR);
        }
        else if (IsHorizontalDividerHovered()) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_HRESIZE);
        }
        else if (IsVerticalDividerHovered()) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_VRESIZE);
        }
        // Hovering dividers
        else if (GetEditorState() == EditorState::RESIZING_HORIZONTAL_VERTICAL) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_CROSSHAIR);
        }
        else if (GetEditorState() == EditorState::RESIZING_HORIZONTAL) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_HRESIZE);
        }
        else if (GetEditorState() == EditorState::RESIZING_VERTICAL) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_VRESIZE);
        }
        else if (GetEditorState() == EditorState::PLACEMENT ||
                 GetEditorState() == EditorState::PLACE_CHRISTMAS_LIGHTS ||
                 GetEditorState() == EditorState::PLACE_DOBERMANN ||
                 GetEditorState() == EditorState::PLACE_FENCE ||
                 GetEditorState() == EditorState::PLACE_POWER_POLES ||
                 GetEditorState() == EditorState::PLACE_DDGI_VOLUME ||
                 GetEditorState() == EditorState::PLACE_DOOR ||
                 GetEditorState() == EditorState::PLACE_HOUSE ||
                 GetEditorState() == EditorState::PLACE_KANGAROO ||
                 GetEditorState() == EditorState::PLACE_PICTURE_FRAME ||
                 GetEditorState() == EditorState::PLACE_MERMAID ||
                 GetEditorState() == EditorState::PLACE_SHARK ||
                 GetEditorState() == EditorState::PLACE_TREE ||
                 GetEditorState() == EditorState::PLACE_WALL ||
                 GetEditorState() == EditorState::PLACE_WINDOW ||
                 GetEditorState() == EditorState::PLACE_PLAYER_CAMPAIGN_SPAWN ||
                 GetEditorState() == EditorState::PLACE_PLAYER_DEATHMATCH_SPAWN ||
                 GetEditorState() == EditorState::PLACE_OBJECT) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_CROSSHAIR);
        }
        // Nothing? Then the arrow
        else {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }
    }
}
