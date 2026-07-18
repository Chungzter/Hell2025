#pragma once

#include "Hell/Math/GLM.h"

#include <cstdint>

// Common

struct PushConstantsFrameResources {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t rendererDataDeviceAddress = 0;
    uint64_t materialsDeviceAddress = 0;
    uint64_t lightsDeviceAddress = 0;
};

// Geometry

struct PushConstantsMaterialResolve {
    PushConstantsFrameResources frame;

    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress = 0;
    uint64_t inputVerticesDeviceAddress = 0;
    uint64_t skinningDispatchGroupsDeviceAddress = 0;
    uint64_t skinningJobsDeviceAddress = 0;

    uint64_t skinningTransformsDeviceAddress = 0;
    uint64_t vertexWeightsDeviceAddress = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t skinnedVerticesDeviceAddress = 0;
    uint64_t materialsDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t useDepthOffset = 0;
};

// Reflectance

struct PushConstantsReflectedRadiance {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress = 0;
    uint64_t rayQueryMeshInstanceDataDeviceAddress = 0;
    uint32_t rayQueryEnabled = 0;
    uint32_t padding0 = 0;
};

// Lighting

struct PushConstantsDeferredLighting {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress = 0;
    uint64_t rayQueryMeshInstanceDataDeviceAddress = 0;
    uint64_t tileLightsDeviceAddress;
    uint32_t rayQueryEnabled = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsHair {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress = 0;
    uint64_t rayQueryMeshInstanceDataDeviceAddress = 0;
    int32_t flashlightCookieTextureIndex = -1;
    uint32_t rayQueryEnabled = 0; // TODO: remove me, you are no longer checking it in GLSL. u should check c++ side and early return if TLAS build fails
};

// Tile culling

struct PushConstantsTileLightCulling {
    PushConstantsFrameResources frame;
    uint64_t tileLightsDeviceAddress;
    uint64_t tileWorldBoundsDeviceAddress;
};

struct PushConstantsTileWorldBounds {
    PushConstantsFrameResources frame;

    uint64_t tileWorldBoundsDeviceAddress;
    int32_t tileXCount;
    int32_t tileYCount;
};

// Scene rendering

struct PushConstantsSkybox {
    PushConstantsFrameResources frame;
};

struct PushConstantsSpriteSheet {
    PushConstantsFrameResources frame;

    uint64_t spriteSheetRenderItemsDeviceAddress = 0;
};

// UI

struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress = 0;
    float renderTargetWidth = 1.0f;
    float renderTargetHeight = 1.0f;
};

// Debug

struct PushConstantsDebug2D {
    float renderTargetWidth = 1.0f;
    float renderTargetHeight = 1.0f;
};

struct PushConstantsDebug3D {
    PushConstantsFrameResources frame;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDebugView {
    PushConstantsFrameResources frame;
};

struct PushConstantsDebugTileView {
    PushConstantsFrameResources frame;
    uint64_t tileLightsDeviceAddress;
};

// DDGI

struct PushConstantsDDGIPointCloudBaseColor {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudTextureInfoDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t textureInfoCount = 0;
};

struct PushConstantsDDGIPointCloudDebug {
    PushConstantsFrameResources frame;

    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t viewportIndex = 0;
};

struct PushConstantsDDGIPointCloudLighting {
    PushConstantsFrameResources frame;

    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t lightCount = 0;
    uint32_t forceUpdate = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIProbeBorder {
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
    uint32_t distanceAtlasStorageImageIndex = 0;
    uint32_t irradianceAtlasStorageImageIndex = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeDebug {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t viewportIndex = 0;
    uint32_t probeOffset = 0;
    uint32_t probeDebugState = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsDDGIProbeDistance {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceIndicesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t distanceAtlasStorageImageIndex = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeDistanceDispatchArgs {
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceDispatchArgsDeviceAddress = 0;
};

struct PushConstantsDDGIProbeDistanceList {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceIndicesDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradiance {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceIndicesDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    float pointCloudSpacing = 0.0f;
    uint32_t irradianceAtlasStorageImageIndex = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsDDGIProbeIrradianceDirtyPointCheck {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradianceDispatchArgs {
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceDispatchArgsDeviceAddress = 0;
};

struct PushConstantsDDGIProbeIrradianceList {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceIndicesDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradianceTexture {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t totalProbes = 0;
    glm::vec3 worldBoundsMin = glm::vec3(0.0f);
    uint32_t probeOffset = 0;
    glm::vec3 worldBoundsMax = glm::vec3(0.0f);
    uint32_t probeAtlasImageIndex = 0;
    uint32_t indirectDiffuseStorageImageIndex = 0;
    uint32_t indirectDiffuseSurfaceStorageImageIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIProbePointIndices {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudGridOffsetsDeviceAddress = 0;
    uint64_t pointCloudGridCountsDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    uint64_t probeIndexCounterDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    glm::vec3 gridMin = glm::vec3(0.0f);
    float gridCellSize = 0.0f;
    glm::ivec3 gridDimensions = glm::ivec3(0);
    uint32_t totalProbes = 0;
};

struct PushConstantsDDGIProbeRelevance {
    PushConstantsFrameResources frame;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeStateUpdate {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t dirtyDoorAABBsDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t dirtyDoorAABBCount = 0;
    float time = 0.0f;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIRaytraceScene {
    PushConstantsFrameResources frame;

    uint64_t houseVertexBufferDeviceAddress = 0;
    uint64_t houseIndexBufferDeviceAddress = 0;
    uint64_t doorVertexBufferDeviceAddress = 0;
    uint64_t doorIndexBufferDeviceAddress = 0;

    float maxRayDistance = 100.0f;
    uint32_t clearOutput = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};
