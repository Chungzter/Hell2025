#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Math/Math.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

namespace Audio = Hell::Audio;

namespace Unloved {

void Player::UpdateFlashlight(float deltaTime) {
    // Toggle on/off
    if (InventoryIsClosed() && PressedFlashlight() && IsAlive()) {
        Audio::PlayAudio("Flashlight.wav", 1.5f);
        m_flashlightOn = !m_flashlightOn;
    }
    // Modifier
    if (!m_flashlightOn) {
        m_flashLightModifier = 0.0f;
    }
    else {
        m_flashLightModifier = Hell::Math::InterpTo(m_flashLightModifier, 1.0f, deltaTime, 10.5f);
    }

    if (!ViewportIsVisible()) {
        return;
    }

    // Prevent NAN direction, which is the case on first spawn
    if (Hell::Math::IsNan(m_flashlightDirection)) {
        m_flashlightDirection = GetCameraForward();
    }

    glm::vec3 rayHitPosition = m_bvhRayResult.hitPosition;

    float distanceToRayHit = glm::distance(rayHitPosition, GetCameraPosition());

    // Centered pos/dir
    glm::vec3 centeredFlashlightPosition = GetCameraPosition();

    //centeredFlashlightPosition += GetCameraUp() * glm::vec3(-0.01f);
    //centeredFlashlightPosition += GetCameraRight() * glm::vec3(0.01f);
    //centeredFlashlightPosition += GetCameraForward() * glm::vec3(0.05f);
    centeredFlashlightPosition += GetCameraForward() * glm::vec3(-0.15f);

    glm::vec3 centeredFlashlightDirection = GetCameraForward();

    // Offset pos/dir
    glm::vec3 offsetFlashlightPosition = centeredFlashlightPosition;
    offsetFlashlightPosition += GetCameraRight() * glm::vec3(0.1f);
    offsetFlashlightPosition -= GetCameraUp() * glm::vec3(m_bobOffsetY * 2);
    glm::vec3 offsetFlashlightDirection = glm::normalize(rayHitPosition - offsetFlashlightPosition);

    // Compute lerp factor
    float maxDistance = 1.0f;
    float t = glm::clamp(distanceToRayHit / maxDistance, 0.0f, 0.75f);

    // Mix between centered and offset based on distance to cam hit
    glm::vec3 flashlightPositionTarget = glm::mix(centeredFlashlightPosition, offsetFlashlightPosition, t);
    glm::vec3 flashlightDirectionTarget = glm::mix(centeredFlashlightDirection, offsetFlashlightDirection, t);

    // If no hit was found then default back to centered
    if (!m_rayHitFound) {
    //if (textureHitPos == glm::vec3(0.0f) && physxRayHitPos == glm::vec3(0.0f)) {
        flashlightPositionTarget = centeredFlashlightPosition;
        flashlightDirectionTarget = centeredFlashlightDirection;
    }

    // Lerp between last pos/dir to the new ones
    float interSpeed = 40;
    m_flashlightPosition = Hell::Math::InterpTo(m_flashlightPosition, flashlightPositionTarget, deltaTime, interSpeed);
    m_flashlightDirection = Hell::Math::InterpTo(m_flashlightDirection, flashlightDirectionTarget, deltaTime, interSpeed);

    m_flashlightPosition = flashlightPositionTarget;

    float aspectRatio = 1.0f;
    //if (RenderDataManager::GetViewportData().size()) {
    //    float viewportWidth = RenderDataManager::GetViewportData()[m_viewportIndex].width;
    //    float viewportHeight = RenderDataManager::GetViewportData()[m_viewportIndex].height;
    //    aspectRatio = viewportWidth / viewportHeight;
    //}

    if (IsInShop()) {
        if (Unloved::World::GetMermaids().size()) {
            Mermaid& mermaid = Unloved::World::GetMermaids()[0];
            m_flashlightDirection = glm::normalize(mermaid.GetPosition() - GetFootPosition());
        }
    }

    // Projection view matrix
    float lightRadius = 20.0f;
    float outerAngle = glm::radians(30.0f);
    glm::vec3 flashlightTargetPosition = m_flashlightPosition + m_flashlightDirection;
    glm::mat4 flashlightViewMatrix = glm::lookAt(m_flashlightPosition, flashlightTargetPosition, GetCameraUp());
    glm::mat4 spotlightProjection = glm::perspective(outerAngle * 2, aspectRatio, 0.05f, lightRadius);
    m_flashlightProjectionView = spotlightProjection * flashlightViewMatrix;

    // Prevent NAN bugs
    if (Hell::Math::IsNan(m_flashlightPosition)) {
        m_flashlightPosition = flashlightPositionTarget;
    }
}

void Player::UpdateFlashlightFrustum() {
    const Resolutions& resolutions = Config::GetResolutions();
    int renderTargetWidth = resolutions.gBuffer.x;
    int renderTargetHeight = resolutions.gBuffer.y;
    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    float viewportWidth = viewport->GetSize().x * renderTargetWidth;
    float viewportHeight = viewport->GetSize().y * renderTargetHeight;
    float aspect = viewportWidth / viewportHeight;
    float nearPlane = 0.01f;
    float farPlane = 10.0f;
    glm::mat4 perspectiveMatrix = glm::perspective(m_cameraZoom, aspect, nearPlane, farPlane);
    glm::mat4 projectionView = perspectiveMatrix * m_camera.GetViewMatrix();
    m_flashlightFrustum.Update(m_flashlightProjectionView);
}

} // namespace Unloved
