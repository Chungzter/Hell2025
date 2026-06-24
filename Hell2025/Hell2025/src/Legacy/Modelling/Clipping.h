#pragma once
#include "Modelling/Types/ClippingCube.h"
#include "Types/House/WallSegment.h"

struct ClippingCubeRayResult {
    bool hitFound = false;
    float distanceToHit = 0;
    glm::vec3 hitPosition = glm::vec3(0.0f);
    ClippingCube* hitClippingCube = nullptr;
};

namespace Clipping {
    void SubtractCubesFromWallSegment(WallSegment& wallSegment, std::vector<ClippingCube>& clippingCubes, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);
    ClippingCubeRayResult CastClippingCubeRay(const glm::vec3& rayOrigin, const glm::vec3 rayDir, std::vector<ClippingCube>& clippingCubes);
}