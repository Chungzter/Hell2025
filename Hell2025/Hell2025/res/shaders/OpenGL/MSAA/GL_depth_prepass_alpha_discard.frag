#version 460

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#if ENABLE_BINDLESS == 1
    #extension GL_ARB_bindless_texture : enable
    readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
    in flat int BaseColorTextureIndex;
#else
    layout (binding = 0) uniform sampler2D baseColorTexture;
#endif

in vec2 v_uv;

void main() {
#if ENABLE_BINDLESS == 1
    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), v_uv);
    vec2 uvTexels = v_uv * textureSize(sampler2D(textureSamplers[BaseColorTextureIndex]), 0);
#else
    vec4 baseColor = texture(baseColorTexture, v_uv);
    vec2 uvTexels = v_uv * textureSize(baseColorTexture, 0);
#endif

    // Calculate mip level to determine distance
    vec2 dx = dFdx(uvTexels);
    vec2 dy = dFdy(uvTexels);
    float mipLevel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));

    float alpha = baseColor.a;

    float alphaPivot = 0.025;     // Center point for contrast boost
    float alphaSharpness = 0.75;  // How aggressively it fattens at a distance
    float alphaBaseBoost = 3.0;   // Increases alpha up close

    // Sharpen alpha threshold as mip level increases
    float boost = max(alphaBaseBoost, mipLevel * alphaSharpness);
    alpha = (alpha - alphaPivot) * boost + alphaPivot;
    alpha = clamp(alpha, 0.0, 1.0);

    // Map alpha sample mask
    uint mask = 0;
    if (alpha > 0.10) mask |= 1; // sample 0
    if (alpha > 0.35) mask |= 2; // sample 1
    if (alpha > 0.65) mask |= 4; // sample 2
    if (alpha > 0.90) mask |= 8; // sample 3

    gl_SampleMask[0] = int(mask);
}