#include "OBB.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace {
    constexpr float MIN_RAY_DISTANCE = 0.001f;
    constexpr float RAY_EPSILON = 1e-8f;
}

OBB::OBB(const AABB& bounds, const glm::mat4& matrix) {
    m_localBounds = bounds;
    m_worldTransform = matrix;
    RecomputeCorners();
}

void OBB::SetTransform(const glm::mat4& matrix) {
    m_worldTransform = matrix;
    RecomputeCorners();
}

void OBB::SetLocalBounds(const AABB& bounds) {
    m_localBounds = bounds;
    RecomputeCorners();
}

OBBRayResult OBB::Raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) const {
    OBBRayResult result;
    if (maxDistance <= MIN_RAY_DISTANCE) {
        return result;
    }

    const glm::mat4 inverseWorldTransform = glm::inverse(m_worldTransform);
    const glm::mat3 normalMatrix = glm::transpose(glm::mat3(inverseWorldTransform));
    const glm::vec3 localOrigin = glm::vec3(inverseWorldTransform * glm::vec4(rayOrigin, 1.0f));
    const glm::vec3 localDir = glm::vec3(inverseWorldTransform * glm::vec4(rayDir, 0.0f));

    if (glm::dot(localDir, localDir) < RAY_EPSILON) {
        return result;
    }

    const glm::vec3& boundsMin = m_localBounds.GetBoundsMin();
    const glm::vec3& boundsMax = m_localBounds.GetBoundsMax();
    glm::vec3 enterNormal = glm::vec3(0.0f);
    glm::vec3 exitNormal = glm::vec3(0.0f);
    float tEnter = -std::numeric_limits<float>::max();
    float tExit = maxDistance;

    for (int axis = 0; axis < 3; axis++) {
        if (std::abs(localDir[axis]) < RAY_EPSILON) {
            if (localOrigin[axis] < boundsMin[axis] || localOrigin[axis] > boundsMax[axis]) {
                return result;
            }
            continue;
        }

        const float inverseDir = 1.0f / localDir[axis];
        float t0 = (boundsMin[axis] - localOrigin[axis]) * inverseDir;
        float t1 = (boundsMax[axis] - localOrigin[axis]) * inverseDir;
        float nearNormalSign = -1.0f;
        float farNormalSign = 1.0f;

        if (t0 > t1) {
            std::swap(t0, t1);
            nearNormalSign = 1.0f;
            farNormalSign = -1.0f;
        }

        if (t0 > tEnter) {
            tEnter = t0;
            enterNormal = glm::vec3(0.0f);
            enterNormal[axis] = nearNormalSign;
        }

        if (t1 < tExit) {
            tExit = t1;
            exitNormal = glm::vec3(0.0f);
            exitNormal[axis] = farNormalSign;
        }

        if (tEnter > tExit) {
            return result;
        }
    }

    if (tExit < MIN_RAY_DISTANCE) {
        return result;
    }

    result.distanceToHit = (tEnter >= MIN_RAY_DISTANCE) ? tEnter : tExit;
    result.hitFound = true;
    result.hitPositionLocal = localOrigin + (localDir * result.distanceToHit);
    result.hitPositionWorld = m_worldTransform * glm::vec4(result.hitPositionLocal, 1.0f);
    result.hitNormalLocal = (tEnter >= MIN_RAY_DISTANCE) ? enterNormal : exitNormal;
    if (glm::dot(result.hitNormalLocal, result.hitNormalLocal) > RAY_EPSILON) {
        result.hitNormalWorld = glm::normalize(normalMatrix * result.hitNormalLocal);
    }
    return result;
}

void OBB::RecomputeCorners() {
    m_corners.clear();
    m_corners.reserve(8);

    const glm::vec3& min = m_localBounds.GetBoundsMin();
    const glm::vec3& max = m_localBounds.GetBoundsMax();

    std::vector<glm::vec3> localPoints = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z)
    };

    for (int i = 0; i < 8; i++) {
        glm::vec4 worldP = m_worldTransform * glm::vec4(localPoints[i], 1.0f);
        m_corners.push_back(glm::vec3(worldP));
    }
}
