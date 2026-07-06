#version 460
#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_uv;

readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

layout(std430, binding = 4) readonly buffer renderItemsBuffer {
    RenderItem renderItems[];
};

#if ENABLE_BINDLESS
// nothing
#else
uniform int u_viewportIndex;
uniform int u_globalInstanceIndex;
#endif

out vec2 v_uv;

void main() {

#if ENABLE_BINDLESS
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    int globalInstanceIndex = instanceOffset + gl_InstanceID;
#else 
    int globalInstanceIndex = u_globalInstanceIndex;
    int viewportIndex = u_viewportIndex;
#endif
    RenderItem renderItem = renderItems[globalInstanceIndex];
    
    mat4 projectionView = viewportData[viewportIndex].projectionView;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 WorldPos = modelMatrix * vec4(a_position, 1.0);
    v_uv = a_uv;

	gl_Position = projectionView * WorldPos;
}
