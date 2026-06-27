#pragma once
#include <Game/Types.h>
#include <Game/CreateInfo.h>
#include "Hell/ResourceManagement/Types/Model.h"

// TODO: remove me
#include "Hell/ResourceManagement/Types/Material.h"

namespace Unloved {

struct ChristmasTree {
    ChristmasTree() = default;
    ChristmasTree(const ChristmasTreeCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    void Update(float deltaTime);
    void CleanUp();
    void CreateRenderItems();

    const glm::vec3& GetPosition() const                    { return m_position; }
    const glm::mat4& GetModelMatrix() const                 { return m_modelMatrix; }
    const std::vector<RenderItem>& GetRenderItems() const   { return m_renderItems; }

private:
    ChristmasTreeCreateInfo m_createInfo;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    Model* m_model = nullptr;
    int32_t m_materialIndex = -1;
    std::vector<RenderItem> m_renderItems;
};
}
