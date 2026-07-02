#version 460

layout(location = 0) flat in uint v_globalInstanceIndex;
layout(location = 0) out uvec2 out_visibility;

void main() {
    out_visibility = uvec2(v_globalInstanceIndex, uint(gl_PrimitiveID));
}
