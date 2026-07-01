#pragma once

#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

namespace Unloved::Editor {

    struct EditorRayResult {
        bool hitFound = false;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
        uint64_t hitObjectId = 0;
        float distanceToHit = std::numeric_limits<float>::max();
    };

    EditorRayResult GetEditorBvhRayResult(float maxRayDistance = 2000.0f);
    EditorRayResult GetEditorHeightMapRayResult(float maxRayDistance = 2000.0f);
    EditorRayResult GetEditorPhysicsRayResult(float maxRayDistance = 2000.0f);
    EditorRayResult GetEditorWorldRayResult(float maxRayDistance = 2000.0f);
}
