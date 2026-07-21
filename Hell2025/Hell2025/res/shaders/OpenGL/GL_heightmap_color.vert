#version 460 core

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_INSTANCE_DATA) buffer renderItemsBuffer { RenderItem renderItems[]; };

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;

#if ENABLE_BINDLESS
out flat int MaterialIndex;
#endif

uniform float u_textureScaling;

#if !ENABLE_BINDLESS
uniform int u_viewportIndex;
uniform int u_globalInstanceIndex;
#endif

void main() {
    TexCoord = vUV * 50.0 * u_textureScaling;

#if ENABLE_BINDLESS
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    int globalInstanceIndex = instanceOffset + gl_InstanceID;
#else
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = u_globalInstanceIndex;
#endif

    RenderItem renderItem = renderItems[globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;

#if ENABLE_BINDLESS
    MaterialIndex = renderItem.materialIndex;
#endif

    vec4 worldPos4 = modelMatrix * vec4(vPosition, 1.0);
    WorldPos = worldPos4.xyz;

    mat3 normalMatrix = transpose(mat3(inverseModelMatrix));
    Normal = normalize(normalMatrix * vNormal);
    Tangent = normalize(normalMatrix * vTangent);
    BiTangent = normalize(cross(Normal, Tangent));
    gl_Position = projectionView * worldPos4;
}
