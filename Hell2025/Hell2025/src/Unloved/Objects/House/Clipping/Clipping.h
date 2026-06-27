#pragma once

#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Objects/House/WallSegment.h"

#include <cstdint>
#include <vector>

namespace HouseBuilder {
    struct ClippingVolume;
}

namespace Unloved {

void SubtractClippingVolumesFromWallSegment(WallSegment& wallSegment, const std::vector<const HouseBuilder::ClippingVolume*>& clippingVolumes, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);

}
