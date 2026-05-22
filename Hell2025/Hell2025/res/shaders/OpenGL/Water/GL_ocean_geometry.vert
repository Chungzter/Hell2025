#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 vPosition;
layout(location = 3) out mat4 v_oceanPatchTransform; // potentially broken

uniform float u_meshSubdivisionFactor;

layout(std430, binding = 10) buffer oceanPatchTransformsBuffer { mat4 oceanPatchTransforms[]; }; // potentially broken

void main () {
    vPosition = inPosition * vec3(u_meshSubdivisionFactor, 0, u_meshSubdivisionFactor);
    v_oceanPatchTransform = oceanPatchTransforms[gl_InstanceID]; // potentially broken
}                                                                
