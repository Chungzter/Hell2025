#pragma once
#include <cstdint>

struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress = 0;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t skinnedVerticesDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress = 0;
    uint64_t inputVerticesDeviceAddress = 0;
    uint64_t skinningTransformsDeviceAddress = 0;
    uint64_t vertexWeightsDeviceAddress = 0;
    uint32_t vertexCount = 0;
    uint32_t baseInputVertex = 0;
    uint32_t baseInputVertexWeight = 0;
    uint32_t baseOutputVertex = 0;
    uint32_t baseTransformIndex = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsMaterialResolve {
    uint64_t renderItemsDeviceAddress = 0;
    uint64_t viewportDataDeviceAddress = 0;
    uint64_t rendererDataDeviceAddress = 0;
    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
};
