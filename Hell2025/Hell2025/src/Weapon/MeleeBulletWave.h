/*#pragma once
#include <Hell/GLM.h>
#include <Game/CreateInfo.h>

struct MeleeBulletWave {
    MeleeBulletWave() = default;
    MeleeBulletWave(MeleeBulletWaveCreateInfo createInfo);
    void Update(float deltaTime);

private:
    glm::vec3 m_startPos = glm::vec3(0.0f);
    glm::vec3 m_localXYPlaneDir = glm::vec3(0.0f);
    float m_startTime = 0.0f;
    float m_maxTime = 0.0f;
    float m_time = 0.0f;
    float m_intervalDuration = 0.0f;
};*/