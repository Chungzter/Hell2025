#pragma once

#include "Unloved/EditorSession/PlacementTools.h"

namespace Unloved::Editor {
    void BeginPlacement(EditorSession::PlacementTool tool);
    void UpdatePlacement();
    void CancelPlacement();
    void FinishPlacement();
}
