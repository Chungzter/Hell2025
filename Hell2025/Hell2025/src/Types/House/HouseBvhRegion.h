#pragma once
#include "Game/Types.h"
#include "GlobalIllumination/PointCloud.h" // For Triangle
#include <vector>

struct HouseBvhRegion {
    void Update(const glm::vec3& worldBoundsMin, const glm::vec3& worldBoundsMax);

    BvhRayResult CastRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float rayLength);
    bool CpuBvhExists();

    uint64_t GetCpuMeshBvhId()  { return m_cpuMeshBvhId; }
    uint64_t GetCpuSceneBvhId() { return m_cpuSceneBvhId; }

private:
    uint64_t m_cpuMeshBvhId = 0;
    uint64_t m_cpuSceneBvhId = 0;
    glm::vec3 m_worldBoundsMin = glm::vec3(0.0f);
    glm::vec3 m_worldBoundsMax = glm::vec3(0.0f);
    std::vector<Triangle> m_triangles;
};
