#version 460 core
layout(origin_upper_left) in vec4 gl_FragCoord;

layout(binding = 0) uniform sampler2D u_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(u_texture, v_uv);
}
