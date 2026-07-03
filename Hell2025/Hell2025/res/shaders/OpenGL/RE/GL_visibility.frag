#version 460

layout(location = 0) out uvec2 VisBufferOut;
layout(location = 0) flat in int v_globalInstanceIndex;

void main() {
    VisBufferOut.x = uint(v_globalInstanceIndex);
    VisBufferOut.y = uint(gl_PrimitiveID);
}
