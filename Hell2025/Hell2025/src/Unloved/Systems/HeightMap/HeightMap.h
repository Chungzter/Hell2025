#pragma once

#include <cstdint>
#include <vector>

namespace Unloved::HeightMap {
    void BuildWorldHeightData(uint32_t chunkCountX, uint32_t chunkCountZ);
    void Clear();

    const std::vector<float>& GetWorldHeightData();
    uint32_t GetWorldTextureWidth();
    uint32_t GetWorldTextureHeight();
}
