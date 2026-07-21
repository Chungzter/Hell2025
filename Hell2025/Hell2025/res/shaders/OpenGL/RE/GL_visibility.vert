#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) flat out int v_globalInstanceIndex;
layout(location = 1) out vec2 v_uv;

readonly restrict layout(std430, binding = SSBO_IDX_INSTANCE_DATA) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform bool u_depthOffset;

void main() {
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    mat4 projectionView = viewportDataArr[viewportIndex].jitteredProjectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;
    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);

    if (u_depthOffset) {
        vec3 cameraPos = viewportDataArr[viewportIndex].viewPos.xyz;
        vec3 awayFromCamera = normalize(worldPos.xyz - cameraPos);
        float depthBiasMeters = 0.01;
        worldPos.xyz += awayFromCamera * depthBiasMeters;
    }

    gl_Position = projectionView * worldPos;

    v_uv = a_uv;
}
