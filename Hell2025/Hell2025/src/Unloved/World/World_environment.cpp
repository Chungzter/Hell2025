#include "World.h"

#include <glm/glm.hpp>

namespace Unloved::World {
    glm::vec3 g_moonlightDirection = glm::normalize(glm::vec3(-0.5f, 0.2f, 0.0f));

    void UpdateEnvironment() {
        g_moonlightDirection = glm::normalize(glm::vec3(-0.5f, 0.2f, 0.0f));
    }

    const glm::vec3& GetMoonlightDirection() {
        return g_moonlightDirection;
    }
}
