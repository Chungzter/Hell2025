#pragma once
#include <cstdint>

struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress = 0;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};
