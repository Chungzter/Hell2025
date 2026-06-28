#include "Geometry.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace Hell::Geometry {

glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& vertexNormal) {
    glm::vec2 uv;

    glm::vec3 absNormal = glm::abs(vertexNormal);

    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        uv.y = vertexPosition.y / absNormal.x;
        uv.x = vertexPosition.z / absNormal.x;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.x > 0) {
            uv.x = 1.0f - uv.x;
        }
    }
    else if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
        uv.x = vertexPosition.x / absNormal.y;
        uv.y = vertexPosition.z / absNormal.y;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.y < 0) {
            uv.x = 1.0f - uv.x;
        }
    }
    else {
        uv.x = vertexPosition.x / absNormal.z;
        uv.y = vertexPosition.y / absNormal.z;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.z < 0) {
            uv.x = 1.0f - uv.x;
        }
    }

    return uv;
}

void SetNormalsAndTangentsFromVertices(Vertex& vert0, Vertex& vert1, Vertex& vert2) {
    glm::vec3& v0 = vert0.position;
    glm::vec3& v1 = vert1.position;
    glm::vec3& v2 = vert2.position;
    glm::vec2& uv0 = vert0.uv;
    glm::vec2& uv1 = vert1.uv;
    glm::vec2& uv2 = vert2.uv;

    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    glm::vec3 normal = glm::normalize(glm::cross(deltaPos1, deltaPos2));
    vert0.normal = normal;
    vert1.normal = normal;
    vert2.normal = normal;
    vert0.tangent = tangent;
    vert1.tangent = tangent;
    vert2.tangent = tangent;
}

glm::vec3 ComputeFaceNormal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;
    return glm::normalize(glm::cross(e1, e2));
}

glm::vec3 Barycentric2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
    glm::vec2 edge0 = v1 - v0;
    glm::vec2 edge1 = v2 - v0;
    glm::vec2 edgeTarget = point - v0;

    float d00 = glm::dot(edge0, edge0);
    float d01 = glm::dot(edge0, edge1);
    float d11 = glm::dot(edge1, edge1);
    float d20 = glm::dot(edgeTarget, edge0);
    float d21 = glm::dot(edgeTarget, edge1);
    float denom = d00 * d11 - d01 * d01;

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

std::vector<uint32_t> GenerateSequentialIndices(int vertexCount) {
    std::vector<uint32_t> indices(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        indices[i] = static_cast<uint32_t>(i);
    }
    return indices;
}

}
