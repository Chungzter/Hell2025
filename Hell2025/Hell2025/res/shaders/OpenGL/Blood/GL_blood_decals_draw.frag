#version 450
#extension GL_ARB_bindless_texture : enable

#include "../../common/constants.glsl"
#include "../../common/flags.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 DecalMaskOut;

layout(binding = 1) uniform sampler2D GBufferNormalXYRoughnessMiscTexture;
layout(binding = 2) uniform sampler2D u_depthTexture;

layout(std430, binding = 0)  readonly restrict buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
layout(std430, binding = 2)  readonly restrict buffer rendererDataBuffer    { RendererData rendererData; };
layout(std430, binding = 3)  readonly restrict buffer viewportDataBuffer    { ViewportData viewportDataArr[]; };
layout(std430, binding = 8)  restrict          buffer tileBloodDecalsBuffer { TileInstanceData tileBloodDecals[]; };
layout(std430, binding = 9)  readonly restrict buffer BloodDecalBuffer      { BloodDecal bloodDecals[]; };
layout(std430, binding = 10) restrict          buffer DecalIndexPool        { uint globalBloodDecalIndices[]; };

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
    uint miscFlags = DecodeMiscFlags(gBufferNormalXYRoughnessMisc.a);
    if ((miscFlags & MISC_FLAG_DYNAMIC_OBJECT) != 0u) discard;

    uint viewportIndex = ComputeViewportIndexFromSplitscreenMode(px, outputImageSize, rendererData.splitscreenMode);
    vec2 screenUV = (vec2(px) + 0.5) / vec2(outputImageSize);
    vec2 viewportUV = ScreenUVToViewportUV(screenUV, viewportDataArr[viewportIndex]);

    ViewportData viewportData = viewportDataArr[viewportIndex];
    mat4 inverseProjectionView = viewportData.inverseProjectionViewReverseZ;

    float depth = texelFetch(u_depthTexture, px, 0).r;
    if (depth <= 0.0) discard;

    vec3 worldPos = ReconstructWorldPos(viewportUV, depth, inverseProjectionView);

    vec3 positionDx = dFdx(worldPos);
    vec3 positionDy = dFdy(worldPos);
    vec3 receiverNormalUnnormalized = cross(positionDx, positionDy);
    float receiverNormalLengthSquared = dot(receiverNormalUnnormalized, receiverNormalUnnormalized);

    // Derivative magnitude changes with distance, so reject only a genuinely
    // degenerate or non-finite surface differential.
    if (!(receiverNormalLengthSquared > 0.0) || isinf(receiverNormalLengthSquared)) discard;

    vec3 receiverNormal = receiverNormalUnnormalized * inversesqrt(receiverNormalLengthSquared);

    float bestMask = 0.0;

    uint tileOffset = tileBloodDecals[tileIndex].offset;

    for (uint i = 0; i < count; ++i) {
        uint decalIdx = globalBloodDecalIndices[tileOffset + i];
        mat4 inverseModelMatrix = bloodDecals[decalIdx].inverseModelMatrix;
        vec3 localPos = (inverseModelMatrix * vec4(worldPos, 1.0)).xyz;
        vec2 aspectScale = vec2(bloodDecals[decalIdx].aspectScaleX, bloodDecals[decalIdx].aspectScaleY);
        vec3 decalHalfExtents = vec3(0.5 * aspectScale.x, 0.5 * BLOOD_DECAL_DEPTH_SCALE, 0.5 * aspectScale.y);

        if (any(greaterThan(abs(localPos), decalHalfExtents))) {
            continue;
        }

        // These decals lie in local XZ, with local Y as the projector normal
        vec3 decalNormal = normalize(
            transpose(mat3(inverseModelMatrix)) * vec3(0.0, 1.0, 0.0)
        );
        if (abs(dot(receiverNormal, decalNormal)) < BLOOD_DECAL_MIN_NORMAL_ALIGNMENT) {
            continue;
        }

        vec2 texCoords = (localPos.xz / aspectScale) + 0.5;
        float a = 0.0;

        int textureIndex = bloodDecals[decalIdx].textureIndex;
        if (textureIndex < 0) continue;
        a = texture(sampler2D(textureSamplers[textureIndex]), texCoords).a;
     
        bestMask = max(bestMask, a);

        if (bestMask >= 0.990) {
            break;
        }
    }

    DecalMaskOut = vec4(vec3(bestMask), 1.0);
}
