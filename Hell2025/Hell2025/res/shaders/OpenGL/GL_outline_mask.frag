#version 460 core
layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 FragOut;

void main() {
    FragOut = vec4(1.0, 1.0, 1.0, 1.0);
}
