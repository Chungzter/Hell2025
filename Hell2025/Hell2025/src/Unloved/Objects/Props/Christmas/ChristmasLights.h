#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/MeshBufferOLD.h"
#include "Unloved/Objects/Exterior/Wire.h"

#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct GPUChristmasLight {
    glm::vec4 position;
    glm::vec4 color;
};

struct ChristmasLightSet {
    ChristmasLightSet() = default;
    ChristmasLightSet(uint64_t id, ChristmasLightsCreateInfo& createInfo, SpawnOffset& spawnOffset);
    ChristmasLightSet(const ChristmasLightSet&) = delete;
    ChristmasLightSet& operator=(const ChristmasLightSet&) = delete;
    ChristmasLightSet(ChristmasLightSet&&) noexcept = default;
    ChristmasLightSet& operator=(ChristmasLightSet&&) noexcept = default;
    ~ChristmasLightSet() = default;

    void AddSegementFromLastPoint(const glm::vec3& nextPoint, float sag);
    void Update(float deltaTime);
    void RecreateLightRenderItems();
    void CleanUp();

    //glm::vec3 m_start;
    //glm::vec3 m_end;
    //float m_sag = 1.0f;
    bool m_spiral = false;
    glm::vec3 sprialTopCenter;
    float spiralRadius;
    float spiralHeight;
    float m_time = 0;

    MeshBufferOLD m_meshBuffer;
    std::vector<glm::vec3> m_wireSegmentPoints;
    std::vector<glm::vec3> m_lightSpawnPoints;

    std::vector<Wire>& GetWires()                                { return m_wires; }
    const std::vector<GPUChristmasLight> GetGPUChristmasLights() { return m_GPUChristmasLights; }
    const std::vector<RenderItem>& GetRenderItems() const        { return m_renderItems; }
    const ChristmasLightsCreateInfo& GetCreateInfo() const       { return m_createInfo; }
    const uint64_t GetObjectId() const                           { return m_objectId; }
    const glm::vec3& GetPosition() const                         { return m_position; }
    const glm::vec3& GetRotation() const                         { return m_rotation; }

private:
    std::vector<RenderItem> m_renderItems;
    std::vector<GPUChristmasLight> m_GPUChristmasLights;
    ChristmasLightsCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f); 
    std::vector<Wire> m_wires;
};
}
