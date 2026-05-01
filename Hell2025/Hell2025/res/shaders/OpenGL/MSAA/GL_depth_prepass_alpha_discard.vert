#version 460

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

#if ENABLE_BINDLESS
out flat int BaseColorTextureIndex;
#else
uniform int u_globalInstanceIndex;
uniform int u_viewportIndex;
#endif

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_uv;

readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };

out vec2 v_uv;

void main() {

#if ENABLE_BINDLESS
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    int globalInstanceIndex = instanceOffset + gl_InstanceID;
    BaseColorTextureIndex = renderItems[globalInstanceIndex].baseColorTextureIndex; 
#else
    int globalInstanceIndex = u_globalInstanceIndex;
    int viewportIndex = u_viewportIndex;
#endif

    v_uv = a_uv;

    RenderItem renderItem = renderItems[globalInstanceIndex]; 
    mat4 projectionView = viewportData[viewportIndex].projectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * worldPos;
}
