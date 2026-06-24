#pragma once
#include "Game/Types.h"

enum struct ParticleType {
    BUBBLE,
};

struct Particle {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float scale = 1.0f;
    float rotation = 1.0f;
    float alphaFade = 1.0f;
    ParticleType m_type = ParticleType::BUBBLE;
};

namespace ParticleManager {

    void Update(float deltaTime);

    std::vector<Particle>& GetParticles();
};