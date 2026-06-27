#pragma once

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace Hell {

    inline float Sanitize(float value, float threshold = 0.002f) {
        if (std::abs(value) < threshold) return 0.0f;
        if (std::abs(value - 1.0f) < threshold) return 1.0f;
        if (std::abs(value + 1.0f) < threshold) return -1.0f;
        return value;
    }

    inline glm::vec3 Sanitize(const glm::vec3& value, float threshold = 0.002f) {
        return glm::vec3(Sanitize(value.x, threshold), Sanitize(value.y, threshold), Sanitize(value.z, threshold));
    }

    inline glm::quat Sanitize(const glm::quat& value, float threshold = 0.002f) {
        glm::quat result = value;
        result.x = Sanitize(value.x, threshold);
        result.y = Sanitize(value.y, threshold);
        result.z = Sanitize(value.z, threshold);
        result.w = Sanitize(value.w, threshold);
        return glm::normalize(result);
    }

    inline void Sanitize(glm::mat4& value, float threshold = 1e-5f) {
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                value[col][row] = Sanitize(value[col][row], threshold);
            }
        }
    }
}
