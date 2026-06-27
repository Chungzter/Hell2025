#pragma once

#include "Unloved/Systems/House/ClippingVolume.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace HouseBuilder {

struct ClipRayResult {
    bool hitFound = false;
    float distanceToHit = std::numeric_limits<float>::max();
    glm::vec3 hitPosition = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    uint64_t objectId = 0;
};

std::vector<const ClippingVolume*> GetClippingVolumes();
void RaycastClippingVolume(const ClippingVolume& clippingVolume, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance, ClipRayResult& result);
ClipRayResult RaycastClippingVolumes(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance);

}
