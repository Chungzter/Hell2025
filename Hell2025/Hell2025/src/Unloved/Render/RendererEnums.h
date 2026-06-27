#pragma once

enum class BlendingMode {
    ALPHA_DISCARD,
    BLENDED,
    DEFAULT,
    HAIR_UNDER_LAYER,
    HAIR,
    TOILET_WATER,
    MIRROR,
    GLASS,
    PLASTIC,
    DO_NOT_RENDER,
    STAINED_GLASS,
    UNDEFINED
};

enum struct ProbeDebugState {
    HIDDEN,
    COLOR,
    DISTANCE,
    DISTANCE_COOL_DOWN,
    IRRADIENCE_COOL_DOWN,
    REVLANCE,
    ACTIVE,
    STATE_COUNT,
};

enum struct RendererMode {
    OLD_DEFERRED,
    RE_STYLE,
    RENDERER_MODE_COUNT
};

enum class RendererOverrideState {
    NONE = 0,
    BASE_COLOR,
    NORMALS,
    RMA,
    ROUGHNESS,
    METALIC,
    AO,
    CAMERA_NDOTL,
    TILE_HEATMAP_LIGHTS,
    TILE_HEATMAP_BLOOD_DECALS,
    TILE_HEATMAP_CHRISTMAS_LIGHTS,
    INDIRECT_DIFFUSE,
    VELOCITY,
    VIS_BUFFER,
    DEPTH,
    WORLD_POSITION,
    EMISSIVE,
    STATE_COUNT,
};