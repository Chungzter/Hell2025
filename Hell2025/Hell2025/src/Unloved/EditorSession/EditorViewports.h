#pragma once

#include "EditorCamera.h"
#include "EditorSessionTypes.h"

#include <cstdint>

struct CreateInfoCollection;

namespace Unloved::EditorSession::Viewports {

    void Init();
    void CancelNavigation();
    void PrepareInitialView(const CreateInfoCollection& createInfoCollection);
    void ApplyInitialView();
    void Update();
    void UpdateInput(bool allowKeyboardInput, bool allowMouseInput);
    void RenderLabels();
    void SetMode(uint32_t viewportIndex, EditorViewportMode mode);
    void SetPivot(const glm::vec3& pivot);

    EditorCamera* GetCameraByIndex(uint32_t viewportIndex);
    const glm::mat4& GetViewMatrix(uint32_t viewportIndex);
    const glm::vec3& GetMouseRayOrigin(uint32_t viewportIndex);
    const glm::vec3& GetMouseRayDirection(uint32_t viewportIndex);
    EditorViewportMode GetMode(uint32_t viewportIndex);
    int32_t GetHoveredViewportIndex();
    bool IsPanning();
    bool IsOrbiting();
    bool IsFlyMode();
    float GetOrthographicSize(uint32_t viewportIndex);
    float GetPerspectiveFov(uint32_t viewportIndex);
}
