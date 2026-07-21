#version 460

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

layout(std430, binding = 3) readonly restrict buffer viewportDataBuffer { ViewportData viewportData[]; };
layout(std430, binding = 4) readonly restrict buffer renderItemsBuffer  { RenderItem renderItems[]; };

centroid out vec2 TexCoord;
centroid out vec4 WorldPos;
centroid out vec3 Normal;
centroid out vec3 Tangent;
centroid out vec3 ViewPos;

out flat int v_globalInstanceIndex;
out flat int v_viewportIndex;

#if !ENABLE_BINDLESS
uniform int u_viewportIndex;
uniform int u_globalInstanceIndex;
#endif

void main() {
#if ENABLE_BINDLESS
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;
    v_viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
#else
    v_globalInstanceIndex = u_globalInstanceIndex;
    v_viewportIndex = u_viewportIndex;
#endif

    RenderItem renderItem = renderItems[v_globalInstanceIndex]; 
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    ViewportData viewportData = viewportData[v_viewportIndex];
    mat4 projectionView = viewportData.jitteredProjectionViewReverseZ;

    Normal = normalize(normalMatrix * vec4(vNormal, 0.0)).xyz;
    Tangent = normalize(modelMatrix * vec4(vTangent, 0.0)).xyz;
    
    TexCoord = vUV;
    
    WorldPos = modelMatrix * vec4(vPosition, 1.0);
    gl_Position = projectionView * WorldPos;
}
