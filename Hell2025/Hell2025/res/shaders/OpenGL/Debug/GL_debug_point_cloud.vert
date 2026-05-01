#version 460 core

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_normal;
layout(location = 2) in vec4 a_directLighting;
layout(location = 3) in vec4 a_baseColor;

uniform int u_viewportIndex;
uniform mat4 u_projectionView;
out vec4 v_normal;
out vec4 v_directLighting;
out vec2 v_uv;
out vec3 v_baseColor;
out vec3 v_worldPos;

void main() {
    vec3 position = a_position.xyz;

    // Offset along normal to get it out of the wall
    position += a_normal.xyz * 0.01;

    gl_Position = u_projectionView * vec4(position, 1.0);

    v_worldPos = a_position.xyz;
    v_normal = a_normal;
    v_directLighting = a_directLighting;
    v_baseColor = a_baseColor.rgb;

    v_uv = vec2(a_baseColor.x, a_baseColor.y); // They're temporarily baked in here
}