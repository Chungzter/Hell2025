#pragma once
#include <cstdint>

struct Material {
    int32_t m_basecolor = -1;
    int32_t m_normal = -1;
    int32_t m_rma = -1;
    int32_t m_emissive = -1;

    int32_t m_opacity = -1;
    int32_t m_hairMaps = -1;
    int32_t m_padding0 = 0;
    int32_t m_padding1 = 0;
};