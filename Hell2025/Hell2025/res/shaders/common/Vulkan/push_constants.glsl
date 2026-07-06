struct PushConstantsUI {
    uint64_t renderItemsDeviceAddress;
};

struct PushConstantsVisibility {
    uint64_t renderItemsDeviceAddress;
    uint64_t viewportDataDeviceAddress;
    uint64_t skinnedVerticesDeviceAddress;
    uint64_t materialsDeviceAddress;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress;
    uint64_t inputVerticesDeviceAddress;
    uint64_t skinningTransformsDeviceAddress;
    uint64_t vertexWeightsDeviceAddress;
    uint vertexCount;
    uint baseInputVertex;
    uint baseInputVertexWeight;
    uint baseOutputVertex;
    uint baseTransformIndex;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct PushConstantsMaterialResolve {
    uint64_t renderItemsDeviceAddress;
    uint64_t viewportDataDeviceAddress;
    uint64_t rendererDataDeviceAddress;
    uint64_t materialsDeviceAddress;
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint vertexCount;
    uint indexCount;
};

struct PushConstantsDeferredLighting {
    uint64_t viewportDataDeviceAddress;
    uint64_t rendererDataDeviceAddress;
    uint64_t lightsDeviceAddress;
    uint64_t materialsDeviceAddress;
    uint64_t rayQueryInstanceDataDeviceAddress;
    uint64_t rayQueryGeometryDataDeviceAddress;
    uint rayQueryEnabled;
    uint padding0;
};
