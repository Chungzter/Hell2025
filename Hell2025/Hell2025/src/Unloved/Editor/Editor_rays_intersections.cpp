#include "Editor.h"

#include "Hell/Physics/Physics.h"

#include "Unloved/Viewport/ViewportManager.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/intersect.hpp>

namespace Unloved::Editor {

    glm::vec3 GetMouseRayPlaneIntersectionPoint(glm::vec3 planeOrigin, glm::vec3 planeNormal) {
        int viewportIndex = Editor::GetHoveredViewportIndex();
        const Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
        const glm::vec3 rayOrigin = Editor::GetMouseRayOriginByViewportIndex(viewportIndex);
        const glm::vec3 rayDir = Editor::GetMouseRayDirectionByViewportIndex(viewportIndex);
        float distanceToHit = 0;
        bool hitFound = glm::intersectRayPlane(rayOrigin, rayDir, planeOrigin, planeNormal, distanceToHit);

        if (!hitFound) {
            return glm::vec3(0.0f);
        }
        else {
            glm::vec3 hitPosition = rayOrigin + (rayDir * distanceToHit);

            // Snap to grid
            hitPosition = glm::round(hitPosition * 10.0f) / 10.0f;

            return hitPosition;
        }
    }
}
