#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 0) flat out int v_globalInstanceIndex;

readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };


void main() {
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    mat4 projectionView = viewportData[viewportIndex].projectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * worldPos;
}
