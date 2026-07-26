#include "EditorScrollBar.h"

#include "EditorCoordinates.h"
#include "EditorUI.h"

#include "Hell/Input.h"

#include <algorithm>
#include <cmath>

namespace Unloved::EditorSession::ScrollBar {
    namespace {
        constexpr int32_t MINIMUM_THUMB_SIZE = 24;

        const glm::vec4 TRACK_COLOR = glm::vec4(0.082353f, 0.074510f, 0.094118f, 1.0f);
        const glm::vec4 THUMB_COLOR = glm::vec4(0.258824f, 0.243137f, 0.286275f, 1.0f);
        const glm::vec4 THUMB_HOVER_COLOR = glm::vec4(0.352941f, 0.333333f, 0.388235f, 1.0f);

        int32_t GetMaximumValue(int32_t contentSize, int32_t visibleSize) {
            return std::max(0, contentSize - visibleSize);
        }

        void RefreshThumb(EditorScrollBar& scrollBar, int32_t contentSize, int32_t visibleSize) {
            const int32_t maximumValue = GetMaximumValue(contentSize, visibleSize);
            const float visibleRatio = static_cast<float>(visibleSize) / static_cast<float>(contentSize);
            const int32_t thumbHeight = std::clamp(static_cast<int32_t>(std::round(static_cast<float>(scrollBar.trackRect.height) * visibleRatio)), std::min(MINIMUM_THUMB_SIZE, scrollBar.trackRect.height), scrollBar.trackRect.height);
            const int32_t thumbTravel = scrollBar.trackRect.height - thumbHeight;
            const float scrollRatio = maximumValue > 0 ? static_cast<float>(scrollBar.value) / static_cast<float>(maximumValue) : 0.0f;

            scrollBar.thumbRect = { scrollBar.trackRect.x, scrollBar.trackRect.y + static_cast<int32_t>(std::round(static_cast<float>(thumbTravel) * scrollRatio)), scrollBar.trackRect.width, thumbHeight };
        }
    }

    void Update(EditorScrollBar& scrollBar, const EditorRect& rect, int32_t contentSize, int32_t visibleSize, bool allowInput) {
        scrollBar.trackRect = rect;
        scrollBar.visible = rect.HasArea() && visibleSize > 0 && contentSize > visibleSize;

        if (!scrollBar.visible) {
            scrollBar.value = 0;
            scrollBar.hovered = false;
            scrollBar.dragging = false;
            return;
        }

        const int32_t maximumValue = GetMaximumValue(contentSize, visibleSize);
        scrollBar.value = std::clamp(scrollBar.value, 0, maximumValue);
        RefreshThumb(scrollBar, contentSize, visibleSize);

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        if (!Hell::Input::LeftMouseDown()) scrollBar.dragging = false;

        scrollBar.hovered = allowInput && scrollBar.trackRect.Contains(mousePosition);
        if (allowInput && Hell::Input::LeftMousePressed() && scrollBar.thumbRect.Contains(mousePosition)) {
            scrollBar.dragging = true;
            scrollBar.dragOffset = mousePosition.y - scrollBar.thumbRect.y;
        }
        else if (allowInput && Hell::Input::LeftMousePressed() && scrollBar.trackRect.Contains(mousePosition)) {
            scrollBar.value += mousePosition.y < scrollBar.thumbRect.y ? -visibleSize : visibleSize;
            scrollBar.value = std::clamp(scrollBar.value, 0, maximumValue);
            RefreshThumb(scrollBar, contentSize, visibleSize);
        }

        if (!allowInput || !scrollBar.dragging || !Hell::Input::LeftMouseDown()) return;

        const int32_t thumbTravel = scrollBar.trackRect.height - scrollBar.thumbRect.height;
        if (thumbTravel <= 0) return;

        const int32_t thumbOffset = std::clamp(mousePosition.y - scrollBar.dragOffset - scrollBar.trackRect.y, 0, thumbTravel);
        scrollBar.value = static_cast<int32_t>(std::round(static_cast<float>(thumbOffset) / static_cast<float>(thumbTravel) * static_cast<float>(maximumValue)));
        RefreshThumb(scrollBar, contentSize, visibleSize);
    }

    void Render(const EditorScrollBar& scrollBar) {
        if (!scrollBar.visible) return;

        UI::DrawSolidRect(scrollBar.trackRect, TRACK_COLOR);
        UI::DrawSolidRect(scrollBar.thumbRect, scrollBar.hovered || scrollBar.dragging ? THUMB_HOVER_COLOR : THUMB_COLOR);
    }

    bool WantsMouseCapture(const EditorScrollBar& scrollBar) {
        return scrollBar.visible && (scrollBar.hovered || scrollBar.dragging);
    }
}
