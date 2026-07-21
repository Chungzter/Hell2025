#ifndef VULKAN_PUSH_CONSTANTS_GLSL
#define VULKAN_PUSH_CONSTANTS_GLSL

#include "../types.glsl"
#include "VK_types.glsl"

layout(buffer_reference, scalar) readonly buffer RenderItemBuffer {
    RenderItem renderItems[];
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportData[];
};

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, scalar) readonly buffer LightBuffer {
    Light lights[];
};

layout(buffer_reference, scalar) readonly buffer SpriteSheetRenderItemBuffer {
    SpriteSheetRenderItem spriteSheetRenderItems[];
};

layout(buffer_reference, scalar) readonly buffer RenderItemUIBuffer {
    RenderItemUI uiRenderItems[];
};

layout(buffer_reference, scalar) buffer TileLightsBuffer {
    TileLights tileLights[];
};

layout(buffer_reference, scalar) buffer TileWorldBoundsBuffer {
    TileWorldBounds tileWorldBounds[];
};

layout(buffer_reference, scalar, buffer_reference_align = 8)
readonly buffer FrameAddressTable {
    RenderItemBuffer renderItemBuffer;
    ViewportDataBuffer viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer;
    MaterialBuffer materialBuffer;
    LightBuffer lightBuffer;
    SpriteSheetRenderItemBuffer spriteSheetRenderItemBuffer;
    RenderItemUIBuffer uiRenderItemBuffer;
    TileLightsBuffer tileLightBuffer;
    TileWorldBoundsBuffer tileWorldBoundsBuffer;
};

struct PushConstantsUI {
    FrameAddressTable frame;
    float renderTargetWidth;
    float renderTargetHeight;
};

struct PushConstantsVisibility {
    FrameAddressTable frame;
    uint64_t skinnedVerticesDeviceAddress;
    uint viewportIndex;
    uint useDepthOffset;
};

struct PointShadowFaceData {
    mat4 projectionView;
    vec4 lightPositionRadius;
    uint arrayLayer;
};

struct PushConstantsPointShadow {
    FrameAddressTable frame;
    uint64_t faceDataDeviceAddress;
};

struct PushConstantsSkinning {
    FrameAddressTable frame;
    uint64_t outputVerticesDeviceAddress;
    uint64_t previousSkinnedPositionsDeviceAddress;
    uint64_t inputVerticesDeviceAddress;
    uint64_t skinningDispatchGroupsDeviceAddress;
    uint64_t skinningJobsDeviceAddress;

    uint64_t skinningTransformsDeviceAddress;
    uint64_t previousSkinningTransformsDeviceAddress;
    uint64_t vertexWeightsDeviceAddress;
    uint padding0;
    uint padding1;
};

struct PushConstantsDebugView {
    FrameAddressTable frame;
};

struct PushConstantsDebug3D {
    FrameAddressTable frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsDebug2D {
    FrameAddressTable frame;
    float renderTargetWidth;
    float renderTargetHeight;
};

struct PushConstantsSkybox {
    FrameAddressTable frame;
};

struct PushConstantsMaterialResolve {
    FrameAddressTable frame;

    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint64_t previousSkinnedPositionsDeviceAddress;
    uint vertexCount;
    uint indexCount;
    uint hasPreviousSkinnedPositions;
    uint padding0;
};

struct PushConstantsDeferredLighting {
    FrameAddressTable frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress;
    uint64_t rayQueryMeshInstanceDataDeviceAddress;
    int brdfLutTextureIndex;
    uint padding0;
};

struct PushConstantsIndirectSpecularAMDInput {
    FrameAddressTable frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress;
    uint64_t rayQueryMeshInstanceDataDeviceAddress;
    int blueNoiseTextureIndex;
    uint frameIndex;
    uint samplesPerQuad;
    uint padding0;
    uint64_t ddgiReflectionVolumeDataDeviceAddress;
    uint enableDDGIReflections;
    uint padding1;
};

struct PushConstantsIndirectSpecularAMDReproject {
    FrameAddressTable frame;
    uint historyValid;
    uint padding0;
};

struct PushConstantsIndirectSpecularAMDPrefilter {
    FrameAddressTable frame;
};

struct PushConstantsSpriteSheet {
    FrameAddressTable frame;
};

struct PushConstantsPostProcessing {
    FrameAddressTable frame;
};

struct PushConstantsHair {
    FrameAddressTable frame;
};

struct PushConstantsTileWorldBounds {
    FrameAddressTable frame;
    int tileXCount;
    int tileYCount;
};

struct PushConstantsTileLightCulling {
    FrameAddressTable frame;
};

struct PushConstantsDebugTileView {
    FrameAddressTable frame;
};

struct PushConstantsDDGIRaytraceScene {
    FrameAddressTable frame;

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
    FrameAddressTable frame;
    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudTextureInfoDeviceAddress;
    uint pointCount;
    uint textureInfoCount;
};

struct PushConstantsDDGIPointCloudLighting {
    FrameAddressTable frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint lightCount;
    uint forceUpdate;
    uint padding0;
};

struct PushConstantsDDGIPointCloudDebug {
    FrameAddressTable frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint viewportIndex;
};

struct PushConstantsDDGIProbeDebug {
    FrameAddressTable frame;
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
    FrameAddressTable frame;
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
    FrameAddressTable frame;
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
    FrameAddressTable frame;
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
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeDistanceDispatchArgs {
    FrameAddressTable frame;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeIrradianceDirtyPointCheck {
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceList {
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceDispatchArgs {
    FrameAddressTable frame;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeDistance {
    FrameAddressTable frame;
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
    FrameAddressTable frame;
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
    FrameAddressTable frame;
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
    FrameAddressTable frame;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint totalProbes;
    uint probeOffset;
    uint distanceAtlasStorageImageIndex;
    uint irradianceAtlasStorageImageIndex;
    uint padding2;
};

#endif
