#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstdint>
#include <vector>

#define PROBE_DISTANCE_OCTA_SIZE 16
#define PROBE_DISTANCE_TEXEL_COUNT (PROBE_DISTANCE_OCTA_SIZE * PROBE_DISTANCE_OCTA_SIZE)

struct ProbeColor {
    glm::vec4 sh[9];
};

struct ProbeState {
    glm::vec3 relocationOffset = glm::vec3(0.0f);
    uint32_t padding;

    uint32_t isRelevant; // bool in GLSL
    uint32_t isActive;  // bool in GLSL
    uint32_t distanceCooldown;
    uint32_t irradianceCooldown;
};

struct DDGIVolumeGPU {
    glm::vec3 origin{};
    float probeSpacing{};

    glm::ivec3 probeCounts{};
    int32_t numProbes{};

    glm::vec3 worldBoundsMin{};
    float padding0{};

    glm::vec3 worldBoundsMax{};
    float padding1{};
};

namespace Unloved {

struct DDGISurfaceTriangle {
    glm::vec3 v0 = glm::vec3(0.0f);
    glm::vec3 v1 = glm::vec3(0.0f);
    glm::vec3 v2 = glm::vec3(0.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec2 uv1 = glm::vec2(0.0f);
    glm::vec2 uv2 = glm::vec2(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    int baseColorTextureIndex = -1;
    int rmaTextureIndex = -1;
};

struct DDGIHouseGeometry {
    std::vector<DDGISurfaceTriangle> surfaceTriangles;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

}
