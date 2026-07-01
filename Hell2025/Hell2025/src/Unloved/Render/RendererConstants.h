#pragma once

#define STENCIL_BIT_STATIC       (1u << 0) // Includes alpha discarded
#define STENCIL_BIT_SKINNED      (1u << 1) // Includes alpha discarded
#define STENCIL_BIT_PROCEDUAL    (1u << 2)
#define STENCIL_BIT_STATIC_HAIR  (1u << 3)
#define STENCIL_BIT_SKINNED_HAIR (1u << 4)

#define SHADOW_BIT_NONE            0u
#define SHADOW_BIT_CAST_SHADOW     (1u << 0)
#define SHADOW_BIT_CAST_CSM_SHADOW (1u << 1)
#define SHADOW_BIT_STATIC          (1u << 2)

#define FLASHLIGHT_SHADOWMAP_SIZE 1024
#define SHADOW_MAP_HI_RES_SIZE 2048
#define SHADOW_MAP_CSM_SIZE 1024
#define SHADOW_NEAR_PLANE 0.05f

#define SHADOW_CASCADE_COUNT 5
#define MAX_GPU_LIGHTS 16

#define TEXTURE_ARRAY_SIZE 1024

#define TILE_SIZE 24

#define MAX_GPU_PARTICLES 12000