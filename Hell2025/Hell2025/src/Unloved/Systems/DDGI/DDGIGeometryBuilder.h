#pragma once

#include "Unloved/Systems/DDGI/DDGITypes.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace Unloved::DDGIGeometryBuilder {
    void CollectHouseSurfaceTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<DDGISurfaceTriangle>& triangles);
    DDGIHouseGeometry BuildHouseGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax);
    void BuildHouseMesh(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    std::vector<DDGIDoorProxyInstance> CollectDoorProxyInstances(const glm::vec3& boundsMin, const glm::vec3& boundsMax);
    void BuildDoorProxyMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void CreateDoorProxyBvh();
    void DestroyDoorProxyBvh();
    uint64_t GetDoorProxyBvhId();
}
