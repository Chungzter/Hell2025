#pragma once

#include "Unloved/Editor/Editor_rays.h"
#include "Unloved/EditorSession/PlacementTools.h"

namespace Unloved::Editor {
    void PlaceDirectObject(EditorSession::PlacementTool tool, const EditorRayResult& rayResult, const EditorSession::PlacementToolInfo& toolInfo);
}
