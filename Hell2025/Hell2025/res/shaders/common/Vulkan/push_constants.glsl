struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress;
    uint64_t viewportDataDeviceAddress;
    uint viewportIndex;
    uint padding0;
};
