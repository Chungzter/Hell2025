#pragma once

#include "Hell/BVH/Types.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct Vertex;

namespace Unloved {

struct HouseOccluderTriangle {
    glm::vec3 v0 = glm::vec3(0.0f);
    glm::vec3 v1 = glm::vec3(0.0f);
    glm::vec3 v2 = glm::vec3(0.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec2 uv1 = glm::vec2(0.0f);
    glm::vec2 uv2 = glm::vec2(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    int baseColorTextureIndex = -1;
    int rmaTextureIndex = -1;
};

namespace WorldBVH {
    void UpdateBvhs();
    void MarkStaticSceneBvhDirty();
    void CreateHouseOccluderTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<HouseOccluderTriangle>& triangles);
    void CreateHouseOccluderGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void UpdateHouseLightOccluderBvh();
    BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
    BvhRayResult ClosestHouseLightOccluderHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
}
}
