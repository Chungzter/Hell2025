#pragma once

#include "Unloved/Editor/PlacementTools.h"

namespace Unloved::Editor {
    void BeginPlacement(PlacementTool tool);
    void UpdatePlacement();
    void CancelPlacement();
    void FinishPlacement();
}
