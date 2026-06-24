#pragma once
#include <Game/Types.h>
#include <Game/CreateInfo.h>
#include "Physics/Physics.h"

struct BulletCasing {
    BulletCasing() = default;
    BulletCasing(BulletCasingCreateInfo createInfo);
    float m_audioDelay = 0.0f;
    float m_lifeTime = 0.0f;
    bool m_collisionsEnabled = false;

    void Update(float deltaTime);
    void CleanUp();
    void SubmitRenderItem();
    //void CollisionResponse();
    const glm::mat4& GetModelMatrix();

    uint64_t GetrigidDynamicId()      { return m_rigidDynamicId; }
    uint32_t GetMeshId()            { return m_meshId; }
    uint32_t GetMaterialIndex()     { return m_materialIndex; }

private:
    uint64_t m_rigidDynamicId = 0;
    uint32_t m_materialIndex = 0;
    uint32_t m_meshId = 0;
    glm::mat4 m_modelMatrix = glm::mat4(1);
};
