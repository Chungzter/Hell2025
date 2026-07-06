#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) flat out int v_globalInstanceIndex;
layout(location = 1) out vec2 v_uv;

readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = 4) buffer renderItemsBuffer  { RenderItem renderItems[]; };

uniform bool u_depthOffset;

void main() {
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    mat4 projectionView = viewportDataArr[viewportIndex].projectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);

    gl_Position = projectionView * worldPos;
    
    if (u_depthOffset) {
        gl_Position.z -= 0.0001;
    }
    
    v_uv = a_uv;
}
