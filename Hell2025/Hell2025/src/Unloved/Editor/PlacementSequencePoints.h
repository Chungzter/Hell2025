#pragma once

#include "Unloved/Common/SequencePoint.h"
#include "Unloved/Editor/PlacementTools.h"

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace Unloved::Editor {
    uint64_t CreatePointSequenceObject(PlacementTool tool, const std::vector<SequencePoint>& sequencePoints, const PlacementToolInfo& toolInfo);
    void UpdatePointSequenceObject(PlacementTool tool, uint64_t objectId, const std::vector<SequencePoint>& sequencePoints);
}
