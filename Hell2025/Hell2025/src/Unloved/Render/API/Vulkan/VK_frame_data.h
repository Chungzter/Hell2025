#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

inline constexpr size_t VULKAN_MAX_UI_VERTICES = 262144;
inline constexpr size_t VULKAN_MAX_UI_INDICES = 393216;
inline constexpr size_t VULKAN_MAX_UI_RENDER_ITEMS = 16384;

struct FrameAddressTable {
    uint64_t renderItemBuffer = 0;
    uint64_t viewportDataBuffer = 0;
    uint64_t rendererDataBuffer = 0;
    uint64_t materialBuffer = 0;
    uint64_t lightBuffer = 0;
    uint64_t spriteSheetRenderItemBuffer = 0;
    uint64_t uiRenderItemBuffer = 0;
    uint64_t tileLightBuffer = 0;
    uint64_t tileWorldBoundsBuffer = 0;
};

static_assert(sizeof(FrameAddressTable) == sizeof(uint64_t) * 9);

struct VulkanFrameData {
    struct Buffers {
        uint64_t frameAddressTable = 0;
        uint64_t instanceData = 0;
        uint64_t viewportData = 0;
        uint64_t rendererData = 0;
        uint64_t lights = 0;
        uint64_t materials = 0;
        uint64_t spriteSheetInstanceData = 0;
        uint64_t drawCommands = 0;
        uint64_t pointShadowFaceData = 0;
        uint64_t skinningDispatchGroups = 0;
        uint64_t skinningJobs = 0;
        uint64_t skinningTransforms = 0;
        uint64_t previousSkinningTransforms = 0;
        uint64_t skinnedVertices = 0;
        uint64_t previousSkinnedPositions = 0;
        uint64_t rayQueryInstances = 0;
        uint64_t rayQueryBLASInstanceData = 0;
        uint64_t rayQueryMeshInstanceData = 0;
        uint64_t rayQueryScratch = 0;
        uint64_t ddgiRayQueryScratch = 0;
        uint64_t uiRenderItems = 0;
        uint64_t tileLights = 0;
        uint64_t tileWorldBounds = 0;
    } buffers;

    struct DDGI {
        uint64_t dirtyDoorAABBs = 0;
        uint32_t dirtyDoorAABBCount = 0;
        uint64_t probeIndexCounter = 0;

        uint64_t probeDistanceCounter = 0;
        uint64_t probeDistanceIndices = 0;
        uint64_t probeDistanceDispatchArgs = 0;

        uint64_t probeIrradianceCounter = 0;
        uint64_t probeIrradianceIndices = 0;
        uint64_t probeIrradianceDispatchArgs = 0;
    } ddgi;

    struct GenericMeshes {
        uint64_t ui = 0;
        uint64_t debugLines2D = 0;
        uint64_t debugLines3D = 0;
        uint64_t debugPoints2D = 0;
        uint64_t debugPoints3D = 0;
    } genericMeshes;

    struct AccelerationStructures {
        struct SkinnedBLASSlot {
            uint64_t id = 0;
            uint32_t baseVertex = 0;
            uint32_t baseIndex = 0;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
            uint32_t geometryCount = 0;
            uint64_t geometryHash = 0;
            uint64_t accelerationStructureSize = 0;
            uint64_t buildScratchSize = 0;
            uint64_t updateScratchSize = 0;
            bool built = false;
        };

        uint64_t rayQueryTLAS = 0;
        uint32_t rayQueryTLASInstanceCapacity = 0;
        uint64_t rayQueryTLASScratchSize = 0;
        uint64_t skinnedVertexBufferAddress = 0;
        std::vector<SkinnedBLASSlot> skinnedBLAS;
    } accelerationStructures;
};
