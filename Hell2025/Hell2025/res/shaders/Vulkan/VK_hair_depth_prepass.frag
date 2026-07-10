#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/binding_indices.glsl"
#include "../common/alpha_to_coverage.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in int v_baseColorTextureIndex;

void main() {
    if (v_baseColorTextureIndex < 0) {
        discard;
    }

    uint textureIndex = uint(v_baseColorTextureIndex);
    float alpha = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_uv).a;
    vec2 textureSizePixels = vec2(textureSize(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), 0));

    uint sampleMask = GetHairSampleMask4x(v_uv, textureSizePixels, alpha);
    if (sampleMask == 0u) {
        discard;
    }

    gl_SampleMask[0] = int(sampleMask);
}
