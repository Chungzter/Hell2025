#define MISC_FLAG_DYNAMIC_OBJECT (1u << 0)
#define MISC_FLAG_RESERVED_0     (1u << 1)
#define MISC_FLAG_RESERVED_1     (1u << 2)
#define MISC_FLAG_RESERVED_2     (1u << 3)

#ifdef __cplusplus
#pragma once

static float EncodeMiscFlags(unsigned int flags) {
    return float(flags & 3u) / 3.0f;
}

static unsigned int DecodeMiscFlags(float value) {
    float clamped = value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
    return static_cast<unsigned int>(clamped * 3.0f + 0.5f);
}

#else

float EncodeMiscFlags(uint flags) {
    return float(flags & 3u) / 3.0;
}

uint DecodeMiscFlags(float value) {
    float clamped = value < 0.0 ? 0.0 : value > 1.0 ? 1.0 : value;
    return uint(clamped * 3.0 + 0.5);
}

#endif
