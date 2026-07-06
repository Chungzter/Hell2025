#define MISC_FLAG_DYNAMIC_OBJECT (1u << 0)
#define MISC_FLAG_RESEVERED      (1u << 1)

#define BLENDING_MODE_ALPHA_DISCARD    0u
#define BLENDING_MODE_BLENDED          1u
#define BLENDING_MODE_DEFAULT          2u
#define BLENDING_MODE_HAIR_UNDER_LAYER 3u
#define BLENDING_MODE_HAIR             4u
#define BLENDING_MODE_TOILET_WATER     5u
#define BLENDING_MODE_MIRROR           6u
#define BLENDING_MODE_GLASS            7u
#define BLENDING_MODE_PLASTIC          8u
#define BLENDING_MODE_DO_NOT_RENDER    9u
#define BLENDING_MODE_STAINED_GLASS    10u
#define BLENDING_MODE_UNDEFINED        11u

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
