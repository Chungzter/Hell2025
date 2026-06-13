#pragma once
#include <glm/glm.hpp>

#define MAX_GPU_PARTICLES 12000

struct GpuParticle {
    glm::vec4 position;
    glm::vec4 velocity;

    float rotation;
    float rotationalVelocity;
    float lifeTime = 0.0f;
    uint32_t exists = 0;
};

struct GPUMaterial {
    int basecolor = 0;
    int normal = 0;
    int rma = 0;
    int emissive = 0;

    int opacity = 0;
    int hairMaps = 0;
    int padding;
    int padding2;
};