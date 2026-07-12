struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress;
    float renderTargetWidth;
    float renderTargetHeight;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress;
    uint64_t viewportDataDeviceAddress;
    uint64_t skinnedVerticesDeviceAddress;
    uint64_t materialsDeviceAddress;
    uint viewportIndex;
    uint useDepthOffset;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress;
    uint64_t inputVerticesDeviceAddress;
    uint64_t skinningDispatchGroupsDeviceAddress;
    uint64_t skinningJobsDeviceAddress;

    uint64_t skinningTransformsDeviceAddress;
    uint64_t vertexWeightsDeviceAddress;
    uint padding0;
    uint padding1;
};

struct PushConstantsFrameResources {
    uint64_t renderItemsDeviceAddress;
    uint64_t viewportDataDeviceAddress;
    uint64_t rendererDataDeviceAddress;
    uint64_t materialsDeviceAddress;
    uint64_t lightsDeviceAddress;
};

struct PushConstantsDebugView {
    PushConstantsFrameResources frame;
};

struct PushConstantsSkybox {
    PushConstantsFrameResources frame;
};

struct PushConstantsMaterialResolve {
    PushConstantsFrameResources frame;

    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint vertexCount;
    uint indexCount;
};

struct PushConstantsDeferredLighting {
    PushConstantsFrameResources frame;

    uint64_t rayQueryInstanceDataDeviceAddress;
    uint64_t rayQueryGeometryDataDeviceAddress;
    uint rayQueryEnabled;
    uint padding0;
};

struct PushConstantsHair {
    PushConstantsFrameResources frame;

    uint64_t rayQueryInstanceDataDeviceAddress;
    uint64_t rayQueryGeometryDataDeviceAddress;
    int flashlightCookieTextureIndex;
    uint rayQueryEnabled;
};

struct PushConstantsTileWorldBounds {
    PushConstantsFrameResources frame;
    uint64_t tileWorldBoundsDeviceAddress;
    int tileXCount;
    int tileYCount;
};
