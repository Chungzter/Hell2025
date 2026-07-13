#pragma once

#include <cstdint>

struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress = 0;
    float renderTargetWidth = 1.0f;
    float renderTargetHeight = 1.0f;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t skinnedVerticesDeviceAddress = 0;
    uint64_t materialsDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t useDepthOffset = 0;
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

struct PushConstantsFrameResources {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t rendererDataDeviceAddress = 0;
    uint64_t materialsDeviceAddress = 0;
    uint64_t lightsDeviceAddress = 0;
};

struct PushConstantsDebugView {
    PushConstantsFrameResources frame;
};

struct PushConstantsSkybox {
    PushConstantsFrameResources frame;
};

struct PushConstantsMaterialResolve {
    PushConstantsFrameResources frame;

    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct PushConstantsDeferredLighting {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress = 0;
    uint64_t rayQueryMeshInstanceDataDeviceAddress = 0;
    uint32_t rayQueryEnabled = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsSpriteSheet {
    PushConstantsFrameResources frame;

    uint64_t spriteSheetRenderItemsDeviceAddress = 0;
};

struct PushConstantsHair {
    PushConstantsFrameResources frame;

    uint64_t rayQueryBLASInstanceDataDeviceAddress = 0;
    uint64_t rayQueryMeshInstanceDataDeviceAddress = 0;
    int32_t flashlightCookieTextureIndex = -1;
    uint32_t rayQueryEnabled = 0; // TODO: remove me, you are no longer checking it in GLSL. u should check c++ side and early return if TLAS build fails
};

struct PushConstantsTileWorldBounds {
    PushConstantsFrameResources frame;

    uint64_t tileWorldBoundsDeviceAddress;
    int32_t tileXCount;
    int32_t tileYCount;
};
