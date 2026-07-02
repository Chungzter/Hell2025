#version 460

layout(set = 0, binding = 0) uniform usampler2D u_visibilityTexture;

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
    ivec2 size = textureSize(u_visibilityTexture, 0);
    ivec2 texel = clamp(ivec2(v_uv * vec2(size)), ivec2(0), size - ivec2(1));
    uvec2 visibility = texelFetch(u_visibilityTexture, texel, 0).xy;

    if (visibility.x == 0u && visibility.y == 0u) {
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    uint hash = Hash(visibility.x * 65537u + visibility.y);
    vec3 color = vec3(float(hash & 255u), float((hash >> 8) & 255u), float((hash >> 16) & 255u)) / 255.0;
    out_color = vec4(color, 1.0);
}
