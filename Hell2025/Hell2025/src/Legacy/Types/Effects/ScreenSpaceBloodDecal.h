#pragma once
#include <Game/Types.h>
#include <Game/CreateInfo.h>

struct ScreenSpaceBloodDecal {
    void Init(ScreenSpaceBloodDecalCreateInfo createInfo);

    int GetType()                       { return m_type; }
    glm::mat4 GetModelMatrix()          { return m_modelMatrix; }
    glm::mat4 GetInverseModelMatrix()   { return m_inverseModelMatrix; }

    int m_type = 0;

private:
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    glm::mat4 m_inverseModelMatrix = glm::mat4(1.0f);
};