#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

struct ShadowMapInfo {
    uint64_t lightId = 0;
    int32_t shadowMapIndex = -1;
};

constexpr size_t SHADOW_MAP_HI_RES_MAX_COUNT = 6;
constexpr size_t SHADOW_MAP_HI_RES_RESOLUTION = 1024;

constexpr size_t SHADOW_MAP_LOW_RES_MAX_COUNT = 10;
constexpr size_t SHADOW_MAP_LOW_RES_RESOLUTION = 512;

namespace Unloved::ShadowMapManager {
    void BeginFrame();
    void Update();

    int32_t GetHiResShadowMapIndex(uint64_t lightId);
    int32_t GetLowResShadowMapIndex(uint64_t lightId);

    inline size_t GetShadowMapHiResMaxCount()    { return SHADOW_MAP_HI_RES_MAX_COUNT; }
    inline size_t GetShadowMapLowResMaxCount()   { return SHADOW_MAP_LOW_RES_MAX_COUNT; }
    inline size_t GetShadowMapHiResResolution()  { return SHADOW_MAP_HI_RES_RESOLUTION; }
    inline size_t GetShadowMapLowResResolution() { return SHADOW_MAP_LOW_RES_RESOLUTION; }

    const std::vector<ShadowMapInfo>& GetDirtyHiResShadowMaps();
    const std::vector<ShadowMapInfo>& GetDirtyLowResShadowMaps();
}
