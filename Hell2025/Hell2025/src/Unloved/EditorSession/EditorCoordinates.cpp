#include "EditorCoordinates.h"
#include "EditorViewports.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include <algorithm>
#include <cstdint>

namespace Unloved::EditorSession::Coordinates {

    glm::ivec2 WindowToUI(const glm::ivec2& windowPosition) {
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        const int32_t windowWidth = std::max(1, Hell::BackEnd::GetCurrentWindowWidth());
        const int32_t windowHeight = std::max(1, Hell::BackEnd::GetCurrentWindowHeight());

        return glm::ivec2(static_cast<int32_t>(static_cast<float>(windowPosition.x) * static_cast<float>(resolution.x) / static_cast<float>(windowWidth)), static_cast<int32_t>(static_cast<float>(windowPosition.y) * static_cast<float>(resolution.y) / static_cast<float>(windowHeight)));
    }

    glm::ivec2 GetMousePositionUI() {
        // Hidden fly cursor does not touch editor UI
        if (Viewports::IsFlyMode()) return glm::ivec2(-1);
        return WindowToUI(glm::ivec2(Hell::Input::GetMouseX(), Hell::Input::GetMouseY()));
    }
}
