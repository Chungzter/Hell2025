#pragma once

#include "Viewport.h"

#include <vector>

namespace Unloved::ViewportManager {
    void Init();
    void Update();
    Unloved::Viewport* GetViewportByIndex(int32_t viewportIndex);
    std::vector<Unloved::Viewport>& GetViewports();
}
