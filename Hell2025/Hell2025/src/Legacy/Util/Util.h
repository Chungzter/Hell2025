#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"
#include "Unloved/ObjectId.h"

#include "Hell/Math/AABB.h"
#include "Hell/Render/TextureTypes.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/Types/Animation.h"

#include "Unloved/Bible/Info/ItemInfo.h"
#include "Unloved/Debug/DebugTypes.h"
#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Render/RendererTypes.h"

#include <nlohmann/json.hpp>

#include <type_traits>
#include <vector>

namespace Util {
    // Math
    glm::mat4 CalculateProjectionReverseZ(float fovY_radians, float aspect, float zNear);
    glm::vec3 EulerRotationFromNormal(glm::vec3 normal, glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f));
    float YRotationBetweenTwoPoints(glm::vec3 a, glm::vec3 b);
    glm::mat4 GetRotationMat4FromForwardVector(glm::vec3 forward);
    float EulerYRotationBetweenTwoPoints(glm::vec3 a, glm::vec3 b);
    glm::mat4 RotationMatrixFromForwardVector(glm::vec3 forward, glm::vec3 worldForward, glm::vec3 worldUp);
    glm::vec2 ComputeCentroid2D(const std::vector<glm::vec2>& points);
    std::vector<glm::vec2> SortConvexHullPoints2D(std::vector<glm::vec2>&points);
    std::vector<glm::vec2> ComputeConvexHull2D(std::vector<glm::vec2> points);
    float Cross2D(const glm::vec2& O, const glm::vec2& A, const glm::vec2& B);
    glm::vec3 ClosestPointOnSegmentToRay(const glm::vec3& A, const glm::vec3& B, const glm::vec3& rayOrigin, const glm::vec3& rayDir);
    int RandomInt(int min, int max);
    void InterpolateQuaternion(glm::quat& Out, const glm::quat& Start, const glm::quat& End, float pFactor);
    float FInterpTo(float current, float target, float deltaTime, float interpSpeed);
    glm::vec3 LerpVec3(glm::vec3 current, glm::vec3 target, float deltaTime, float interpSpeed);
    float RandomFloat(float min, float max);
    glm::ivec2 WorldToScreenCoords(const glm::vec3& worldPos, const glm::mat4& viewProjection, int screenWidth, int screenHeight, bool flipY = false);
    //glm::ivec2 WorldToScreenCoordsOrtho(const glm::vec3& worldPos, const glm::mat4& orthoMatrix, int screenWidth, int screenHeight, bool flipY = false);
    bool IsNan(float value);
    bool IsNan(const glm::vec2& value);
    bool IsNan(const glm::vec3& value);
    bool IsNan(const glm::vec4& value);
    bool IsNaN(const glm::mat4& matrix);
    bool IsPointInTriangle2D(const glm::vec2& pt, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2);
    std::vector<glm::vec3> GetBeizerPointsFromControlPoints(const std::vector<glm::vec3>& controlPoints, float spacing);
    bool HoveredLine(glm::ivec2 mouseCoords, glm::ivec2 p1, glm::ivec2 p2, float threshold);
    float ChristmasLerp(float start, float end, float t);
    std::vector<glm::vec3> GenerateSagPoints(const glm::vec3& start, const glm::vec3& end, int numPoints, float sagAmount);
    std::vector<glm::vec3> GenerateCirclePoints(const glm::vec3& center, const glm::vec3& forward, float radius, int numPoints);
    float FractalNoise1D(float x, int32_t seed);
    inline float DegToRad(float degrees) { return degrees * (HELL_PI / 180.0f); }
    glm::mat4 CreateObliqueProjection(const glm::mat4& projection, const glm::mat4& view, const glm::vec4& plane);

    // Rendering
    void UpdateRenderItemAABB(RenderItem& renderItem);
    void UpdateRenderItemAABBFastA(RenderItem& renderItem);
    void UpdateRenderItemAABBFastB(RenderItem& renderItem);
    AABB ComputeWorldAABB(glm::vec3& localAabbMin, glm::vec3& localAabbMax, glm::mat4& modelMatrix);
    glm::mat4 GetLightSpaceMatrix(const glm::mat4& viewMatrix, glm::vec3 lightDir, const float viewportWidth, const float viewportHeight, const float fov, const float nearPlane, const float farPlane);
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
    std::vector<glm::mat4> GetLightProjectionViews(const glm::mat4& viewMatrix, glm::vec3 lightDir, std::vector<float>& shadowCascadeLevels, const float viewportWidth, const float viewportHeight, const float fov);

    // Animation
    //int FindAnimatedNodeIndex(float AnimationTime, const AnimatedNode* animatedNode);
    const AnimatedNode* FindAnimatedNode(Animation* animation, const char* NodeName);
    void CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode);
    void CalcInterpolatedScale(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode);
    void CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const AnimatedNode* animatedNode);
    glm::mat4 Mat4InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ);
    glm::mat4 Mat4InitRotateTransform(float RotateX, float RotateY, float RotateZ);
    glm::mat4 Mat4InitTranslationTransform(float x, float y, float z);
   inline void SetBitState(uint32_t& bitmask, uint32_t bit, bool state) {
        bitmask = (bitmask & ~bit) | (state ? bit : 0);
    }

    // Templates
    template<typename In, typename MinIn, typename MaxIn, typename MinOut, typename MaxOut>
    inline float MapRange(In inValue, MinIn minInRange, MaxIn maxInRange, MinOut minOutRange, MaxOut maxOutRange) {
        static_assert(std::is_arithmetic<In>::value && std::is_arithmetic<MinIn>::value && std::is_arithmetic<MaxIn>::value && std::is_arithmetic<MinOut>::value && std::is_arithmetic<MaxOut>::value, "MapRange requires arithmetic types");
        float iv = static_cast<float>(inValue);
        float minI = static_cast<float>(minInRange);
        float maxI = static_cast<float>(maxInRange);
        float minO = static_cast<float>(minOutRange);
        float maxO = static_cast<float>(maxOutRange);
        float x = (iv - minI) / (maxI - minI);
        return minO + (maxO - minO) * x;
    }

}
