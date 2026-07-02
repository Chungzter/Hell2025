#pragma once

#include "Unloved/Systems/House/ClippingVolume.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace Unloved::HouseBuilder {

    void MarkDirty();
    void RebuildAll();
    void RebuildIfDirty();

    bool IsDirty();

    void RecreateAllProceduralWallMesh();
    void RecreateAllProcedularWorldPlaneMesh();
    void RecreateAllWeatherBoards();
    void RecreateAllWallTrims();
    void RecreateAllHangingLightCords();
    void RemoveAllWeatherBoards();

}
