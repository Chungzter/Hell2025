#pragma once

#include "Unloved/Editor/Editor_rays.h"
#include "Unloved/Editor/PlacementTools.h"

namespace Unloved::Editor {
    void PlaceDirectObject(PlacementTool tool, const EditorRayResult& rayResult, const PlacementToolInfo& toolInfo);
}
