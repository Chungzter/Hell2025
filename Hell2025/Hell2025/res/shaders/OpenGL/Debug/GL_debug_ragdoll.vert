#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_projectionView;
uniform mat4 u_model;

out vec3 v_normal;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    v_normal = normalize(normalMatrix * a_normal);

	gl_Position = u_projectionView * u_model * vec4(a_position, 1.0);
}