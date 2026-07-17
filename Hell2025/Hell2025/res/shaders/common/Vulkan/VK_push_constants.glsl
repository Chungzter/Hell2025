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

struct PushConstantsDebug3D {
    PushConstantsFrameResources frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsDebug2D {
    float renderTargetWidth;
    float renderTargetHeight;
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

    uint64_t rayQueryBLASInstanceDataDeviceAddress;
    uint64_t rayQueryMeshInstanceDataDeviceAddress;
    uint rayQueryEnabled;
    uint padding0;
};

struct PushConstantsSpriteSheet {
    PushConstantsFrameResources frame;
    uint64_t spriteSheetRenderItemsDeviceAddress;
};

struct PushConstantsHair {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress;
    uint64_t rayQueryMeshInstanceDataDeviceAddress;
    int flashlightCookieTextureIndex;
    uint rayQueryEnabled;
};

struct PushConstantsTileWorldBounds {
    PushConstantsFrameResources frame;
    uint64_t tileWorldBoundsDeviceAddress;
    int tileXCount;
    int tileYCount;
};

struct PushConstantsDDGIRaytraceScene {
    PushConstantsFrameResources frame;

    uint64_t houseVertexBufferDeviceAddress;
    uint64_t houseIndexBufferDeviceAddress;
    uint64_t doorVertexBufferDeviceAddress;
    uint64_t doorIndexBufferDeviceAddress;

    float maxRayDistance;
    uint clearOutput;
    uint padding0;
    uint padding1;
};

struct PushConstantsDDGIPointCloudBaseColor {
    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudTextureInfoDeviceAddress;
    uint pointCount;
    uint textureInfoCount;
};

struct PushConstantsDDGIPointCloudLighting {
    PushConstantsFrameResources frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint lightCount;
    uint forceUpdate;
    uint padding0;
};

struct PushConstantsDDGIPointCloudDebug {
    PushConstantsFrameResources frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint viewportIndex;
};

struct PushConstantsDDGIProbeDebug {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint viewportIndex;
    uint probeOffset;
    uint probeDebugState;
    uint padding0;
    uint padding1;
};

struct PushConstantsDDGIProbePointIndices {
    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudGridOffsetsDeviceAddress;
    uint64_t pointCloudGridCountsDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    uint64_t probeIndexCounterDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    vec3 gridMin;
    float gridCellSize;
    ivec3 gridDimensions;
    uint totalProbes;
};

struct PushConstantsDDGIProbeStateUpdate {
    uint64_t probeStatesDeviceAddress;
    uint64_t dirtyDoorAABBsDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint dirtyDoorAABBCount;
    float time;
    uint padding0;
};

struct PushConstantsDDGIProbeRelevance {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct PushConstantsDDGIProbeDistanceList {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeDistanceDispatchArgs {
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeIrradianceDirtyPointCheck {
    uint64_t probeStatesDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceList {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceDispatchArgs {
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeDistance {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceIndicesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint distanceAtlasStorageImageIndex;
    uint padding1;
    uint padding2;
};

struct PushConstantsDDGIProbeIrradiance {
    uint64_t pointCloudDeviceAddress;
    uint64_t probeStatesDeviceAddress;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceIndicesDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    float pointCloudSpacing;
    uint irradianceAtlasStorageImageIndex;
    uint padding1;
};

struct PushConstantsDDGIProbeIrradianceTexture {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint totalProbes;
    vec3 worldBoundsMin;
    uint probeOffset;
    vec3 worldBoundsMax;
    uint probeAtlasImageIndex;
    uint indirectDiffuseStorageImageIndex;
    uint indirectDiffuseSurfaceStorageImageIndex;
    uint padding0;
};

struct PushConstantsDDGIProbeBorder {
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint totalProbes;
    uint probeOffset;
    uint distanceAtlasStorageImageIndex;
    uint irradianceAtlasStorageImageIndex;
    uint padding2;
};
