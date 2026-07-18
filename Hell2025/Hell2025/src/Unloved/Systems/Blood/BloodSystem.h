#pragma once

#include "Unloved/Objects/Renderables/VATInstance.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

struct TestParticle {
    TestParticle() = default;
    TestParticle(const glm::vec3& position, const glm::vec3& velocity);
    void Update(float deltaTime);
    void DebugDraw(int32_t randomSeed);

    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_positionPrev = glm::vec3(0.0f);
    glm::vec3 m_velocity = glm::vec3(0.0f);
    glm::vec3 m_finalRestingNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_gravity = -9.8f;
    float m_lifeTime = 0.0f;
    bool m_stopped = false;
};

namespace Unloved::BloodSystem {

    void BeginFrame();
    void Update();

    void SpawnVatBlood(const glm::vec3& position, const glm::vec3& forward, float scale, uint64_t parentHitObjectId);

    const std::vector<VATRenderItem>& GetVATRenderItems();
    const std::vector<TestParticle>& GetTestParticles();

}