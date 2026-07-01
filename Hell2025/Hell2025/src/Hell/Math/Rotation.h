#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Hell::Math {

    inline glm::vec3 EulerRotationFromNormal(const glm::vec3& normal, const glm::vec3& forward = glm::vec3(0.0f, 0.0f, 1.0f)) {
        glm::vec3 normalizedNormal = glm::normalize(normal);
        glm::quat q = glm::rotation(forward, normalizedNormal);
        glm::vec3 euler = glm::eulerAngles(q);
        euler.z = 0.0f;
        return euler;
    }

    inline float YawBetweenPoints(const glm::vec3& a, const glm::vec3& b) {
        float deltaX = b.x - a.x;
        float deltaZ = b.z - a.z;
        float thetaRadians = std::atan2(deltaZ, deltaX);
        return -thetaRadians;
    }

    inline glm::mat4 RotationMatrixFromForward(const glm::vec3& forward, const glm::vec3& worldForward, const glm::vec3& worldUp) {
        (void)worldUp;
        glm::vec3 normalizedForward = glm::normalize(forward);
        glm::vec3 normalizedWorldForward = glm::normalize(worldForward);
        glm::quat q = glm::rotation(normalizedWorldForward, normalizedForward);
        return glm::mat4_cast(q);
    }
}
