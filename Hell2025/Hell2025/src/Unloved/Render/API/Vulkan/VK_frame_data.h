#pragma once
#include <cstddef>
#include <cstdint>

inline constexpr size_t VULKAN_MAX_UI_VERTICES = 262144;
inline constexpr size_t VULKAN_MAX_UI_INDICES = 393216;
inline constexpr size_t VULKAN_MAX_UI_RENDER_ITEMS = 16384;

struct VulkanFrameData {
    struct Buffers {
        uint64_t instanceData = 0;
        uint64_t viewportData = 0;
        uint64_t rendererData = 0;
        uint64_t drawCommands = 0;
        uint64_t skinningTransforms = 0;
        uint64_t skinnedVertices = 0;
        uint64_t uiRenderItems = 0;
    } buffers;
};
