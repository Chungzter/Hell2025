#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Objects/House/Clipping/ClippingCube.h"
#include "Unloved/Objects/House/WallSegment.h"

#include <cstdint>
#include <vector>

namespace Unloved {

struct ClippingCubeRayResult {
    bool hitFound = false;
    float distanceToHit = 0;
    glm::vec3 hitPosition = glm::vec3(0.0f);
    ClippingCube* hitClippingCube = nullptr;
};

void SubtractCubesFromWallSegment(WallSegment& wallSegment, std::vector<ClippingCube>& clippingCubes, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);
ClippingCubeRayResult CastClippingCubeRay(const glm::vec3& rayOrigin, const glm::vec3 rayDir, std::vector<ClippingCube>& clippingCubes);

}
