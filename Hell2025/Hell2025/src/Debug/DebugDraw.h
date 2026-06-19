#pragma once
#include "Camera/Frustum.h"
#include "Math/AABB.h"
#include "Math/OBB.h"

namespace DebugDraw {
    void BeginFrame();
    void UploadVertexData();

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color);
    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1);
    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawOBB(const OBB& obb, const glm::vec4& color);
    void DrawFrustum(const Frustum& frustum, const glm::vec4& color);
    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
}