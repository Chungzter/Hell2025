#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;

layout(std430, binding = 3) readonly restrict buffer viewportDataBuffer { ViewportData viewportData[]; };
layout(std430, binding = 4) readonly restrict buffer renderItemsBuffer  { RenderItem renderItems[]; };

centroid out vec2 v_texCoord;
centroid out vec4 v_worldPos;
centroid out vec3 v_normal;
centroid out vec3 v_tangent;

out flat int v_globalInstanceIndex;
out flat int v_viewportIndex;

void main() {
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    v_globalInstanceIndex = instanceOffset + gl_InstanceID;
    v_viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;

    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    ViewportData viewportData = viewportData[v_viewportIndex];
    mat4 projectionView = viewportData.jitteredProjectionViewReverseZ;

    v_normal = normalize(normalMatrix * vec4(a_normal, 0.0)).xyz;
    v_tangent = normalize(modelMatrix * vec4(a_tangent, 0.0)).xyz;

    v_texCoord = a_uv;

    v_worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * v_worldPos;
}
