#pragma once

#include "Hell/BVH/Types.h"

#include <glm/glm.hpp>
#include <cstdint>

namespace Unloved {

namespace WorldBVH {
    void UpdateBvhs();
    void MarkStaticSceneBvhDirty();
    void RebuildHouseLightOcclusionBvh();
    BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
    BvhRayResult ClosestHouseLightOcclusionHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
}
}
