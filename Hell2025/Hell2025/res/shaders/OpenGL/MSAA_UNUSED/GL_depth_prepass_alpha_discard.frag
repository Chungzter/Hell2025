#version 460
#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#if ENABLE_BINDLESS == 1
    #extension GL_ARB_bindless_texture : enable
layout(origin_upper_left) in vec4 gl_FragCoord;
    readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
    in flat int OpacityTextureIndex;
#else
    layout(binding = 0) uniform sampler2D opacityTexture;
#endif

in vec2 v_uv;

void main() {
#if ENABLE_BINDLESS == 1
    sampler2D opacitySampler = sampler2D(textureSamplers[OpacityTextureIndex]);
    float alpha = texture(opacitySampler, v_uv).r;
    alpha = texture(opacitySampler, v_uv).a;
    vec2 textureSizePixels = vec2(textureSize(opacitySampler, 0));
#else
    float alpha = texture(opacityTexture, v_uv).r;
    vec2 textureSizePixels = vec2(textureSize(opacityTexture, 0));
#endif

    vec2 dx = dFdx(v_uv) * textureSizePixels;
    vec2 dy = dFdy(v_uv) * textureSizePixels;
    float mipLevel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));

    float alphaPivot = 0.025;
    float alphaSharpness = 0.75;
    float alphaBaseBoost = 1.0;

    float boost = max(alphaBaseBoost, mipLevel * alphaSharpness);
    alpha = clamp((alpha - alphaPivot) * boost + alphaPivot, 0.0, 1.0);

    uint mask =
        (uint(alpha > 0.10) << 0) |
        (uint(alpha > 0.35) << 1) |
        (uint(alpha > 0.65) << 2) |
        (uint(alpha > 0.90) << 3);

    gl_SampleMask[0] = int(mask);
}
