#include "Player.h"
#include "Config/Config.h"
#include "Managers/MirrorManager.h"
#include "Viewport/ViewportManager.h"

glm::ivec2 Player::GetViewportCenter() {
    const Viewport* viewport = ViewportManager::GetViewportByIndex(m_viewportIndex);
    if (!viewport->IsVisible()) return glm::ivec2(0, 0);

    const Resolutions& resolutions = Config::GetResolutions();
    int width = resolutions.ui.x * viewport->GetSize().x;
    int height = resolutions.ui.y * viewport->GetSize().y;
    int xLeft = resolutions.ui.x * viewport->GetPosition().x;
    int xRight = xLeft + width;
    int yTop = resolutions.ui.y * (1.0f - viewport->GetPosition().y - viewport->GetSize().y);
    int yBottom = yTop + height;
    int centerX = xLeft + (width / 2);
    int centerY = yTop + (height / 2);
    return glm::ivec2(centerX, centerY);
}


float Player::GetTargetWalkingSpeed() {
    if (m_crouching) {
        return m_crouchingSpeed;
    }
    else if (m_running) {
        return m_runningSpeed;
    }
    else {
        return m_walkingSpeed;
    }
}

uint64_t Player::GetClosestMirrorId() {
    uint64_t mirrorId = 0;
    float closestDistance = 9999999;

    for (Mirror& mirror : MirrorManager::GetMirrors()) {
        if (mirror.IsFacingViewportCamera(m_viewportIndex)) {
            float distanceToPlayer = glm::distance(mirror.GetWorldCenter(), GetCameraPosition());

            if (distanceToPlayer < closestDistance) {
                closestDistance = distanceToPlayer;
                mirrorId = mirror.GetObjectId();
            }
        }
    }

    return mirrorId;
}