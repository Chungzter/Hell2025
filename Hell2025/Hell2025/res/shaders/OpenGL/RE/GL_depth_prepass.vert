#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 a_position;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_INSTANCE_DATA) buffer renderItemsBuffer  { RenderItem renderItems[]; };

uniform int u_globalInstanceIndex;
uniform int u_viewportIndex;

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
    mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * worldPos;
}
