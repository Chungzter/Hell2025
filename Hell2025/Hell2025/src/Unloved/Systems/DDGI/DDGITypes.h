#pragma once

#include "Hell/Math/GLM.h"

#include <cstdint>

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
