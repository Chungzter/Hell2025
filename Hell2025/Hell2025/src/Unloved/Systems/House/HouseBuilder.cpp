#include "HouseBuilder.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace HouseBuilder {

void RaycastClippingVolume(const ClippingVolume& clippingVolume, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance, ClipRayResult& result) {
    const OBBRayResult hit = clippingVolume.GetOBB().Raycast(rayOrigin, rayDir, maxDistance);

    // If no ray hit, or a hit was found greater or equal than the one already stored in ray result, then bail
    if (!hit.hitFound || hit.distanceToHit >= result.distanceToHit) {
        return;
    }

    result.hitFound = true;
    result.distanceToHit = hit.distanceToHit;
    result.hitPosition = hit.hitPositionWorld;
    result.hitNormal = hit.hitNormalWorld;
    result.objectId = clippingVolume.GetOwnerObjectId();
}

std::vector<const ClippingVolume*> GetClippingVolumes() {
    std::vector<const ClippingVolume*> clippingVolumes;

    for (const Unloved::Door& door : Unloved::World::GetDoors()) {
        clippingVolumes.push_back(&door.GetClippingVolume());
    }

    for (const Unloved::Window& window : Unloved::World::GetWindows()) {
        clippingVolumes.push_back(&window.GetClippingVolume());
    }

    return clippingVolumes;
}

ClipRayResult RaycastClippingVolumes(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) {
    ClipRayResult result;

    // Update ray result with the closest hit against all door clipping volumes
    for (const Unloved::Door& door : Unloved::World::GetDoors()) {
        RaycastClippingVolume(door.GetClippingVolume(), rayOrigin, rayDir, maxDistance, result);
    }

    // Update ray result with the closest hit against all window clipping volumes
    for (const Unloved::Window& window : Unloved::World::GetWindows()) {
        RaycastClippingVolume(window.GetClippingVolume(), rayOrigin, rayDir, maxDistance, result);
    }

    return result;
}

}
