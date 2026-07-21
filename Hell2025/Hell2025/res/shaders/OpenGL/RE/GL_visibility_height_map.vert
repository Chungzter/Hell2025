#version 460

#include "../../common/types.glsl"
#include "../../common/constants.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 a_position;

layout(location = 0) flat out int v_globalInstanceIndex;
layout(location = 2) out vec3 v_worldPosition;

readonly restrict layout(std430, binding = SSBO_IDX_INSTANCE_DATA) buffer renderItemsBuffer { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

void main() {
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    mat4 projectionView = viewportDataArr[viewportIndex].jitteredProjectionViewReverseZ;
    v_worldPosition = (renderItem.modelMatrix * vec4(a_position, 1.0)).xyz;
    gl_Position = projectionView * vec4(v_worldPosition, 1.0);
}
