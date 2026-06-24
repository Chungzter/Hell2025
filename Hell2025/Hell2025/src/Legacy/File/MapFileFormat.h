#pragma once

#include <cstdint>

inline constexpr char HELL_MAP_SIGNATURE[] = "HELL_MAP";

#pragma pack(push, 1)
struct MapHeader {
    char signature[32];
    uint32_t version;
    uint32_t chunkCountX;
    uint32_t chunkCountZ;
    uint32_t createInfoJsonLength;
    uint32_t additionalJsonLength;
};
#pragma pack(pop)
