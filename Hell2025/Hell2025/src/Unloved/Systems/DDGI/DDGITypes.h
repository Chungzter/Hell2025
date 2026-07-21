#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Math/AABB.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstdint>
#include <vector>

#define PROBE_DISTANCE_OCTA_SIZE 16
#define PROBE_DISTANCE_TEXEL_COUNT (PROBE_DISTANCE_OCTA_SIZE * PROBE_DISTANCE_OCTA_SIZE)

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

    uint32_t probeOffset{};
    uint32_t padding2{};
    uint32_t padding3{};
    uint32_t padding4{};
};

struct DDGIReflectionVolumeGPU {
    DDGIVolumeGPU volume{};
    uint32_t probeAtlasImageIndex{};
    uint32_t padding0{};
    uint32_t padding1{};
    uint32_t padding2{};
};

struct DDGIReflectionVolumeBufferHeaderGPU {
    uint64_t probeStatesDeviceAddress{};
    uint32_t volumeCount{};
    uint32_t padding0{};
};

static_assert(sizeof(DDGIVolumeGPU) == 80);
static_assert(sizeof(DDGIReflectionVolumeGPU) == 96);
static_assert(sizeof(DDGIReflectionVolumeBufferHeaderGPU) == 16);

namespace Unloved {

struct DDGIProbeUpdateCandidate {
    uint64_t volumeId = 0;
    float nearestCameraDistance = 0.0f;
    uint32_t framesSinceLastProbeUpdate = 0;
};

struct DDGIDoorProxyInstance {
    uint64_t objectId = 0;
    AABB worldAabb;
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

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
