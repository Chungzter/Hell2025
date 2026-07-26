#pragma once

#include <glm/vec2.hpp>

namespace Unloved::EditorSession::Coordinates {

    glm::ivec2 WindowToUI(const glm::ivec2& windowPosition);
    glm::ivec2 GetMousePositionUI();
}
