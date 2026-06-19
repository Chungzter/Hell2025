#include "ClippingCube.h"
#include "Debug/DebugDraw.h"
#include "Util/Util.h"

void ClippingCube::Update(const Transform& transform) {
    m_transform = transform;
    m_modelMatrix = m_transform.to_mat4();

    // Corners
    m_corners.resize(8);
    for (int i = 0; i < 8; i++) {
        float x = (i & 1) ? 0.5f : -0.5f;
        float y = (i & 2) ? 0.5f : -0.5f;
        float z = (i & 4) ? 0.5f : -0.5f;
        glm::vec4 localPos(x, y, z, 1.0f);
        m_corners[i] = glm::vec3(m_modelMatrix * localPos);
    }
    
    // AABB
    m_aabb = Util::GetAABBFromPoints(m_corners);
}

void ClippingCube::DrawDebugCorners(glm::vec4 color) {
    for (glm::vec3& corner : m_corners) {
        DebugDraw::DrawPoint(corner, color);
    }
}

void ClippingCube::DrawDebugEdges(glm::vec4 color) {
    int edges[12][2] = {
        {0, 1}, {0, 2}, {0, 4},
        {1, 3}, {1, 5},
        {2, 3}, {2, 6},
        {3, 7},
        {4, 5}, {4, 6},
        {5, 7},
        {6, 7}
    };
    for (int i = 0; i < 12; i++) {
        DebugDraw::DrawLine(m_corners[edges[i][0]], m_corners[edges[i][1]], color, true);
    }
}