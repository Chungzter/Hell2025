#version 450
#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#if ENABLE_BINDLESS == 1
    #extension GL_ARB_bindless_texture : enable
#endif

#include "../../common/constants.glsl"
#include "../../common/flags.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 DecalMaskOut;

layout(binding = 0) uniform sampler2D GBufferRMATexture;
layout(binding = 1) uniform sampler2D GBufferNormalXYRoughnessMiscTexture;
layout(binding = 2) uniform sampler2D u_depthTexture;

#if ENABLE_BINDLESS == 1
    readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer {
        uvec2 textureSamplers[];
    };
#else
    layout(binding = 3) uniform sampler2D DecalTex0;
    layout(binding = 4) uniform sampler2D DecalTex1;
    layout(binding = 5) uniform sampler2D DecalTex2;
    layout(binding = 6) uniform sampler2D DecalTex3;
#endif

layout(std430, binding = 2)  readonly restrict  buffer rendererDataBuffer    { RendererData rendererData; };
layout(std430, binding = 3)  readonly restrict  buffer viewportDataBuffer    { ViewportData viewportDataArr[]; };
layout(std430, binding = 8)  restrict           buffer tileBloodDecalsBuffer { TileInstanceData tileBloodDecals[]; };
layout(std430, binding = 9)  readonly restrict  buffer BloodDecalBuffer      { BloodDecal bloodDecals[]; };
layout(std430, binding = 10) restrict           buffer DecalIndexPool        { uint globalBloodDecalIndices[]; };

uniform int u_tileXCount;
uniform int u_tileYCount;

void main() {
	ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uvec2 tileCoords = uvec2(px) / TILE_SIZE;
    uint tileIndex = tileCoords.y * u_tileXCount + tileCoords.x;
    uint count = tileBloodDecals[tileIndex].count;

    // Skip if this tile has no decals
    if (count == 0) discard;

    vec4 gBufferNormalXYRoughnessMisc = texelFetch(GBufferNormalXYRoughnessMiscTexture, px, 0);

    // Do nothing on walls (assuming Y is up)
    vec3 normal = DecodeNormal(gBufferNormalXYRoughnessMisc.rg);
    if (abs(normal.y) < 0.5) discard;

    uint miscFlags = DecodeMiscFlags(gBufferNormalXYRoughnessMisc.a);
    if ((miscFlags & MISC_FLAG_DYNAMIC_OBJECT) != 0u) discard;

    uint viewportIndex = ComputeViewportIndexFromSplitscreenMode(px, outputImageSize, rendererData.splitscreenMode);
    vec2 screenUV = (vec2(px) + 0.5) / vec2(outputImageSize);
    vec2 viewportUV = ScreenUVToViewportUV(screenUV, viewportDataArr[viewportIndex]);

    ViewportData viewportData = viewportDataArr[viewportIndex];
    mat4 inverseProjectionView = viewportData.inverseProjectionViewReverseZ;

    float depth = texelFetch(u_depthTexture, px, 0).r;
    vec3 worldPos = ReconstructWorldPos(viewportUV, depth, inverseProjectionView);

    float bestMask = 0.0;

    uint tileOffset = tileBloodDecals[tileIndex].offset;

    for (uint i = 0; i < count; ++i) {
        uint decalIdx = globalBloodDecalIndices[tileOffset + i];

        vec3 localPos = (bloodDecals[decalIdx].inverseModelMatrix * vec4(worldPos, 1.0)).xyz;

        // Explicit clipping
        if (any(greaterThan(abs(localPos), vec3(0.5, 0.5, 0.5)))) {
            continue;
        }

        int textureIndex = bloodDecals[decalIdx].textureIndex;
        vec2 texCoords = localPos.xz + 0.5;

        #if ENABLE_BINDLESS == 1
            float a = texture(sampler2D(textureSamplers[textureIndex]), texCoords).a;
            bestMask = max(bestMask, a);
        #endif

        if (bestMask >= 0.990) {
         //   bestMask = 1.0;
            break;
        }
    }

    DecalMaskOut = vec4(vec3(bestMask), 1.0);
}
