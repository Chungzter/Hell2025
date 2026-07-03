#version 460
layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 WorldPosOut;

in vec3 v_worldPos;

void main() {
    WorldPosOut = vec4(v_worldPos, 1.0);
}
