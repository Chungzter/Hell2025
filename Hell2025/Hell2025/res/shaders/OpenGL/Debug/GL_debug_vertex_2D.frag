#version 450
layout(origin_upper_left) in vec4 gl_FragCoord;
in vec3 Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(Color, 1.0);
}
