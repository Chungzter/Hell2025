#pragma once

#include "Hell/Math/AABB.h"
#include "Hell/Math/Transform.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace Unloved {

struct ClippingCube {
    void Update(const Hell::Transform& transform);
    void DrawDebugCorners(const glm::vec4& color);
    void DrawDebugEdges(const glm::vec4& color);
    
    const std::vector<glm::vec3>& GetCorners()  const { return m_corners; };
    const glm::vec3 GetPosition()               const { return m_transform.position; }
    const glm::vec3 GetHalfExtents()            const { return m_transform.scale * 0.5f; }
    const glm::mat4 GetModelMatrix()            const { return m_modelMatrix; }
    const AABB& GetAABB()                       const { return m_aabb; }

private:
    Hell::Transform m_transform;
    glm::mat4 m_modelMatrix;
    std::vector<glm::vec3> m_corners;
    AABB m_aabb;
};

}
