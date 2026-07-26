#pragma once

#include "Unloved/Common/SequencePoint.h"
#include "Unloved/EditorSession/PlacementTools.h"

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace Unloved::Editor {
    uint64_t CreatePointSequenceObject(EditorSession::PlacementTool tool, const std::vector<SequencePoint>& sequencePoints, const EditorSession::PlacementToolInfo& toolInfo);
    void UpdatePointSequenceObject(EditorSession::PlacementTool tool, uint64_t objectId, const std::vector<SequencePoint>& sequencePoints);
}
