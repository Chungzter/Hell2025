#pragma once

#include "Hell/Render/VertexAttributes.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Hell::Geometry {
    glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& vertexNormal);
    void SetNormalsAndTangentsFromVertices(Vertex& vert0, Vertex& vert1, Vertex& vert2);
    glm::vec3 ComputeFaceNormal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);
    glm::vec3 Barycentric2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2);
    std::vector<uint32_t> GenerateSequentialIndices(int vertexCount);
}
