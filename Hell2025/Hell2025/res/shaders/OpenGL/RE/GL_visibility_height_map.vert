#version 460

#include "../../common/types.glsl"
#include "../../common/constants.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 a_position;

layout(location = 0) flat out int v_sceneRenderItemIndex;
layout(location = 2) out vec3 v_worldPosition;

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform int u_viewportIndex;

void main() {
    uint drawIndex = uint(gl_BaseInstance + gl_InstanceID);
    v_sceneRenderItemIndex = int(drawRenderItemIndices[drawIndex]);

    RenderItem renderItem = sceneRenderItems[v_sceneRenderItemIndex];
    mat4 projectionView = viewportDataArr[u_viewportIndex].jitteredProjectionViewReverseZ;
    v_worldPosition = (renderItem.modelMatrix * vec4(a_position, 1.0)).xyz;
    gl_Position = projectionView * vec4(v_worldPosition, 1.0);
}
