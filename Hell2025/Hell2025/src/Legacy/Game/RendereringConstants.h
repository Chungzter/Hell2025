#pragma once
#include <cstdint>

#define STENCIL_BIT_STATIC       (1u << 0) // Includes alpha discarded
#define STENCIL_BIT_SKINNED      (1u << 1) // Includes alpha discarded
#define STENCIL_BIT_PROCEDUAL    (1u << 2)
#define STENCIL_BIT_STATIC_HAIR  (1u << 3)
#define STENCIL_BIT_SKINNED_HAIR (1u << 4)

// #define STENCIL_REF_STATIC       STENCIL_BIT_STATIC
// #define STENCIL_REF_SKINNED      STENCIL_BIT_SKINNED
// #define STENCIL_REF_PROCEDUAL    STENCIL_BIT_PROCEDUAL
// #define STENCIL_REF_STATIC_HAIR  STENCIL_BIT_STATIC_HAIR
// #define STENCIL_REF_SKINNED_HAIR STENCIL_BIT_SKINNED_HAIR

//#define STENCIL_REF_STATIC_HAIR  (STENCIL_BIT_STATIC  | STENCIL_BIT_HAIR)
//#define STENCIL_REF_SKINNED_HAIR (STENCIL_BIT_SKINNED | STENCIL_BIT_HAIR)

//constexpr uint32_t SHADOW_BIT_POINT_LIGHT = (1u << 0);
//constexpr uint32_t SHADOW_BIT_MOON_LIGHT  = (1u << 1);
//constexpr uint32_t SHADOW_BIT_STATIC      = (1u << 2);

#define SHADOW_BIT_NONE            0u
#define SHADOW_BIT_CAST_SHADOW     (1u << 0)
#define SHADOW_BIT_CAST_CSM_SHADOW (1u << 1)
#define SHADOW_BIT_STATIC          (1u << 2)

/*

uint32_t shadowFlags = SHADOW_BIT_NONE;

// enable standard shadows
shadowFlags |= SHADOW_BIT_CAST_SHADOWS;

// clear csm shadows
shadowFlags &= ~SHADOW_BIT_CAST_CSM_SHADOWS;

// flip the static state
shadowFlags ^= SHADOW_BIT_STATIC;

// checking if static bit is set
if ((shadowFlags & SHADOW_BIT_STATIC) != 0u) {

}

// verify two both types are active
if ((shadowFlags & (SHADOW_BIT_CAST_SHADOWS | SHADOW_BIT_CAST_CSM_SHADOWS)) == (SHADOW_BIT_CAST_SHADOWS | SHADOW_BIT_CAST_CSM_SHADOWS)) {

}

*/
