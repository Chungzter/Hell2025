#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];

layout(location = 0) flat in uint v_globalInstanceIndex;
layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in int v_baseColorTextureIndex;
layout(location = 0) out uvec2 out_visibility;

void main() {
    if (v_baseColorTextureIndex >= 0) {
        float alpha = texture(sampler2D(textures[nonuniformEXT(uint(v_baseColorTextureIndex))], samplers[0]), v_uv).a;
        if (alpha < 0.25) discard;
    }

    out_visibility = uvec2(v_globalInstanceIndex, uint(gl_PrimitiveID));
}
