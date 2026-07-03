#version 460
#extension GL_ARB_bindless_texture : require
#include "../../common/types.glsl"

layout(location = 0) out uvec2 FragmentOutput;

layout(location = 0) flat in int v_globalInstanceIndex;
layout(location = 1) in vec2 v_uv;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };

const float bayerMatrix[16] = float[16](
    0.0,    0.5,    0.125,  0.625,
    0.75,   0.25,   0.875,  0.375,
    0.1875, 0.6875, 0.0625, 0.5625,
    0.9375, 0.4375, 0.8125, 0.3125
);

uniform uint u_frameCount;

void main() {

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    float alpha = texture(sampler2D(textureSamplers[renderItem.baseColorTextureIndex]), v_uv).a;
    //float alpha = texture(sampler2D(textureSamplers[renderItem.opacityTextureIndex]), v_uv).r;

    bool useStochasticDiscard = false;
    float hardAlphaCutoff = 0.25;

    if (useStochasticDiscard) {
        ivec2 pixelCoords = ivec2(gl_FragCoord.xy);

        // Offset jitter coords over time
        ivec2 temporalOffset = ivec2(
            int(u_frameCount % 4),
            int((u_frameCount / 4) % 4)
        );

        ivec2 jitteredCoords = pixelCoords + temporalOffset;
        uint bayerIndex = ((jitteredCoords.y & 3) << 2) | (jitteredCoords.x & 3);
        float ditherThreshold = bayerMatrix[bayerIndex];

        // Stochastic Discard
        float baseCutoff = 0.001;
        if (alpha - baseCutoff < ditherThreshold) {
            discard;
        }
    }
    else {
        // Hard Discard
        if (alpha < hardAlphaCutoff) {
            discard;
        }
    }

    FragmentOutput.x = uint(v_globalInstanceIndex);
    FragmentOutput.y = uint(gl_PrimitiveID);
}
