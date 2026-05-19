#version 460

layout(location = 0) out uvec2 FragmentOutput;
layout(location = 0) flat in int v_globalInstanceIndex;

void main() {
    FragmentOutput.x = uint(v_globalInstanceIndex);
    FragmentOutput.y = uint(gl_PrimitiveID);
}