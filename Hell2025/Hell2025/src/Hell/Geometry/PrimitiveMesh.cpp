#include "PrimitiveMesh.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace Hell::PrimitiveMesh {

std::vector<Vertex> GenerateRingVertices(float sphereRadius, float ringThickness, int segments, int thicknessSegments) {
    std::vector<Vertex> vertices;
    for (int i = 0; i < segments; ++i) {
        float angle = glm::two_pi<float>() * i / segments;
        glm::vec3 ringCenter = glm::vec3(
            sphereRadius * std::cos(angle),
            sphereRadius * std::sin(angle),
            0.0f
        );

        for (int j = 0; j < thicknessSegments; ++j) {
            float thicknessAngle = glm::two_pi<float>() * j / thicknessSegments;
            glm::vec3 offset = glm::vec3(
                ringThickness * std::cos(thicknessAngle) * std::cos(angle),
                ringThickness * std::cos(thicknessAngle) * std::sin(angle),
                ringThickness * std::sin(thicknessAngle)
            );

            Vertex& vertex = vertices.emplace_back();
            vertex.position = ringCenter + offset;
            vertex.normal = glm::normalize(offset);
            vertex.tangent = glm::normalize(glm::vec3(-std::sin(angle), std::cos(angle), 0.0f));
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateRingIndices(int segments, int thicknessSegments) {
    std::vector<uint32_t> indices;
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < thicknessSegments; ++j) {
            int nextI = (i + 1) % segments;
            int nextJ = (j + 1) % thicknessSegments;
            uint32_t v0 = i * thicknessSegments + j;
            uint32_t v1 = nextI * thicknessSegments + j;
            uint32_t v2 = i * thicknessSegments + nextJ;
            uint32_t v3 = nextI * thicknessSegments + nextJ;

            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v2);
            indices.push_back(v1);
            indices.push_back(v3);
        }
    }

    return indices;
}

std::vector<Vertex> GenerateSphereVertices(float radius, int segments) {
    std::vector<Vertex> vertices;
    segments = std::max(segments, 4);

    float thetaStep = glm::two_pi<float>() / segments;
    float phiStep = glm::pi<float>() / segments;

    for (int i = 0; i <= segments; ++i) {
        float phi = i * phiStep;
        for (int j = 0; j <= segments; ++j) {
            float theta = j * thetaStep;
            glm::vec3 position = glm::vec3(
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)
            );
            glm::vec3 normal = glm::normalize(position);
            glm::vec3 tangent = glm::normalize(glm::vec3(
                -radius * std::sin(phi) * std::sin(theta),
                0.0f,
                radius * std::sin(phi) * std::cos(theta)
            ));

            Vertex& vertex = vertices.emplace_back();
            vertex.position = position;
            vertex.normal = normal;
            vertex.tangent = tangent;
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateSphereIndices(int segments) {
    std::vector<uint32_t> indices;
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int nextI = i + 1;
            int nextJ = (j + 1) % (segments + 1);
            uint32_t v0 = i * (segments + 1) + j;
            uint32_t v1 = nextI * (segments + 1) + j;
            uint32_t v2 = i * (segments + 1) + nextJ;
            uint32_t v3 = nextI * (segments + 1) + nextJ;

            indices.push_back(v2);
            indices.push_back(v1);
            indices.push_back(v0);
            indices.push_back(v3);
            indices.push_back(v1);
            indices.push_back(v2);
        }
    }

    return indices;
}

std::vector<Vertex> GenerateConeVertices(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    segments = std::max(segments, 3);

    Vertex& apex = vertices.emplace_back();
    apex.position = glm::vec3(0.0f, height, 0.0f);
    apex.normal = glm::normalize(glm::vec3(0.0f, height, 0.0f));
    apex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);

    float angleStep = glm::two_pi<float>() / segments;
    for (int i = 0; i < segments; ++i) {
        float angle = i * angleStep;
        glm::vec3 position(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
        glm::vec3 normal = glm::normalize(glm::vec3(position.x, height / 2.0f, position.z));
        glm::vec3 tangent = glm::normalize(glm::vec3(-std::sin(angle), 0.0f, std::cos(angle)));

        Vertex& baseVertex = vertices.emplace_back();
        baseVertex.position = position;
        baseVertex.normal = normal;
        baseVertex.tangent = tangent;
    }

    Vertex& baseCenter = vertices.emplace_back();
    baseCenter.position = glm::vec3(0.0f, 0.0f, 0.0f);
    baseCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    baseCenter.tangent = glm::vec3(1.0f, 0.0f, 0.0f);

    return vertices;
}

std::vector<uint32_t> GenerateConeIndices(int segments) {
    std::vector<uint32_t> indices;

    for (int i = 0; i < segments; ++i) {
        uint32_t apexIndex = 0;
        uint32_t baseIndex1 = i + 1;
        uint32_t baseIndex2 = (i + 1) % segments + 1;

        indices.push_back(baseIndex2);
        indices.push_back(baseIndex1);
        indices.push_back(apexIndex);
    }

    uint32_t centerIndex = segments + 1;
    for (int i = 0; i < segments; ++i) {
        uint32_t baseIndex1 = i + 1;
        uint32_t baseIndex2 = (i + 1) % segments + 1;

        indices.push_back(baseIndex1);
        indices.push_back(baseIndex2);
        indices.push_back(centerIndex);
    }

    return indices;
}

std::vector<Vertex> GenerateCylinderVertices(float radius, float height, int subdivisions) {
    std::vector<Vertex> vertices;
    const float angleStep = glm::two_pi<float>() / subdivisions;

    vertices.push_back(Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back(Vertex(glm::vec3(x, 0.0f, z), glm::vec3(0.0f, -1.0f, 0.0f)));
    }

    vertices.push_back(Vertex(glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back(Vertex(glm::vec3(x, height, z), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        glm::vec3 normal = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        vertices.push_back(Vertex(glm::vec3(x, 0.0f, z), normal));
        vertices.push_back(Vertex(glm::vec3(x, height, z), normal));
    }

    return vertices;
}

std::vector<uint32_t> GenerateCylinderIndices(int subdivisions) {
    std::vector<uint32_t> indices;

    for (int i = 1; i <= subdivisions; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i % subdivisions + 1);
    }

    int topCenterIndex = subdivisions + 2;
    for (int i = 1; i <= subdivisions; ++i) {
        indices.push_back(topCenterIndex);
        indices.push_back(topCenterIndex + i);
        indices.push_back(topCenterIndex + (i % subdivisions) + 1);
    }

    int sideStartIndex = (subdivisions + 2) * 2;
    for (int i = 0; i < subdivisions; ++i) {
        int bottomIndex = sideStartIndex + i * 2;
        int topIndex = bottomIndex + 1;

        indices.push_back(bottomIndex);
        indices.push_back(topIndex);
        indices.push_back(bottomIndex + 2);
        indices.push_back(topIndex);
        indices.push_back(topIndex + 2);
        indices.push_back(bottomIndex + 2);
    }

    return indices;
}

std::vector<Vertex> GenerateCubeVertices() {
    std::vector<Vertex> vertices;
    glm::vec3 normals[] = {
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f, -1.0f},
        { 1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
    };
    glm::vec3 positions[] = {
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f,  -0.5f,  0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f,  0.5f},
    };

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 4; ++j) {
            vertices.push_back(Vertex(positions[i * 4 + j], normals[i]));
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateCubeIndices() {
    return {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
}

}
