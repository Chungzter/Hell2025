#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_UINT_TEXTURES) uniform utexture2D uintTextures[];

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

uint Hash(uint value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

void main() {
    ivec2 size = textureSize(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    ivec2 texel = clamp(ivec2(v_uv * vec2(size)), ivec2(0), size - ivec2(1));
    uvec2 visibility = texelFetch(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), texel, 0).xy;

    if (visibility.x == 0u && visibility.y == 0u) {
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    uint hash = Hash(visibility.x * 65537u + visibility.y);
    vec3 color = vec3(float(hash & 255u), float((hash >> 8) & 255u), float((hash >> 16) & 255u)) / 255.0;
    out_color = vec4(color, 1.0);
}
