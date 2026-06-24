#pragma once
#include <Game/CreateInfo.h>
#include "Types/Generics/Wire.h"
#include "Types/Renderer/MeshNodes.h"

struct PowerPoleSet {
    PowerPoleSet() = default;
    PowerPoleSet(uint64_t id, PowerPoleSetCreateInfo& createInfo, SpawnOffset& spawnOffset);
    PowerPoleSet(const PowerPoleSet&) = delete;
    PowerPoleSet& operator=(const PowerPoleSet&) = delete;
    PowerPoleSet(PowerPoleSet&&) noexcept = default;
    PowerPoleSet& operator=(PowerPoleSet&&) noexcept = default;
    ~PowerPoleSet() = default;

    void Init();
    void AddControlPoint(const glm::vec2& controlPoint2D);
    void Update();
    void CleanUp();

    const std::vector<RenderItem>& const GetRenderItems();

    std::vector<Wire>& GetWires() { return m_wires; }

    const uint64_t GetObjectId() const                  { return m_objectId; }
    const PowerPoleSetCreateInfo& GetCreateInfo() const { return m_createInfo; }

private:
    uint64_t m_objectId = 0;
    PowerPoleSetCreateInfo m_createInfo;
    SpawnOffset m_spawnOffset;

    std::vector<glm::vec3> m_finalPositions;
    std::vector<glm::vec3> m_wirePositionsBackA;
    std::vector<glm::vec3> m_wirePositionsBackB;
    std::vector<glm::vec3> m_wirePositionsBackC;
    std::vector<glm::vec3> m_wirePositionsBackD;
    std::vector<glm::vec3> m_wirePositionsFrontA;
    std::vector<glm::vec3> m_wirePositionsFrontB;
    std::vector<glm::vec3> m_wirePositionsFrontC;
    std::vector<glm::vec3> m_wirePositionsFrontD;
    std::vector<RenderItem> m_renderItems;
    MeshNodes m_meshNodes; 
    std::vector<Wire> m_wires;
};