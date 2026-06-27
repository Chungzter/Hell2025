#include "Editor.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Viewport/ViewportManager.h"

#include "Unloved/Config/Config.h"

namespace Unloved::Editor {

    PhysXRayResult GetMouseRayPhsyXHitPosition() {
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(GetHoveredViewportIndex());
        if (!viewport) return PhysXRayResult();

        // Cast physx ray
        float maxRayDistance = 2000;
        glm::vec3 rayOrigin = GetMouseRayOriginByViewportIndex(GetHoveredViewportIndex());
        glm::vec3 rayDir = GetMouseRayDirectionByViewportIndex(GetHoveredViewportIndex());
        return Hell::Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, true);
    }

    float GetScalingFactor(int targetSizeInPixels) {
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(GetHoveredViewportIndex());
        if (!viewport) return 0.0f;

        const Resolutions& resolutions = Config::GetResolutions();

        int renderTargetWidth = resolutions.gBuffer.x;
        int renderTargetHeight = resolutions.gBuffer.y;
        float viewportWidth = viewport->GetSize().x * renderTargetWidth;
        float viewportHeight = viewport->GetSize().y * renderTargetHeight;

        if (viewport->IsOrthographic()) {
            float m_aspect = viewportWidth / viewportHeight;
            float left = -viewport->GetOrthoSize() * m_aspect;
            float right = viewport->GetOrthoSize() * m_aspect;
            float bottom = -viewport->GetOrthoSize();
            float top = viewport->GetOrthoSize();
            float worldHeight = top - bottom;
            float worldPerPixel = worldHeight / viewportHeight;
            float gizmoHeightInWorld = (float)targetSizeInPixels * worldPerPixel;
            return gizmoHeightInWorld;
        }
        else {
            Logging::Error() << "Editor::GetScalingFactor() failed because viewport was not orthographic";
        }
    }
}