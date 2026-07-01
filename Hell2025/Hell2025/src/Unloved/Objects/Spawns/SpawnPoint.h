#pragma once

#include "Unloved/Common/CreateInfo.h"

#include <glm/vec3.hpp>

#include <cstdint>

namespace Unloved {

struct SpawnPoint {
    SpawnPoint() = default;
    SpawnPoint(uint64_t id, const SpawnPointCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    SpawnPoint(const SpawnPoint&) = delete;
    SpawnPoint& operator=(const SpawnPoint&) = delete;
    SpawnPoint(SpawnPoint&&) noexcept = default;
    SpawnPoint& operator=(SpawnPoint&&) noexcept = default;
    ~SpawnPoint() = default;

    void CleanUp();
    void DrawDebugCube();

    const glm::vec3& GetPosition() const { return m_createInfo.position; }
    const glm::vec3& GetCamEuler() const { return m_createInfo.camEuler; }
    const SpawnPointCreateInfo& GetCreateInfo() const { return m_createInfo; }

private:
    uint64_t m_objectId = 0;
    SpawnPointCreateInfo m_createInfo;
};
}
