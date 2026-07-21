#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;

readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = 4) buffer renderItemsBuffer  { RenderItem renderItems[]; };

out vec4 v_currPos;
out vec4 v_prevPos;
out vec3 v_normal;
out vec3 v_tangent;
out vec2 v_uv;

out flat int v_globalInstanceIndex;
out flat int v_viewportIndex;

void main() {
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    int globalInstanceIndex = instanceOffset + gl_InstanceID;

    v_globalInstanceIndex = globalInstanceIndex;
    v_viewportIndex = viewportIndex;

    ViewportData viewportData = viewportDataArr[v_viewportIndex];
    mat4 projectionView = viewportData.projectionViewReverseZ;
    mat4 rasterProjectionView = viewportData.jitteredProjectionViewReverseZ;
    mat4 prevProjectionViewReverseZ = viewportData.prevProjectionViewReverseZ;

    RenderItem renderItem = renderItems[globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 prevModelMatrix = renderItem.prevModelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;

    mat4 normalMatrix = transpose(inverseModelMatrix);
    v_normal = normalize(normalMatrix * vec4(a_normal, 0.0)).xyz;
    v_tangent = normalize(modelMatrix * vec4(a_tangent, 0.0)).xyz;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);
    vec4 prevWorldPos = prevModelMatrix * vec4(a_position, 1.0);

    v_currPos = projectionView * worldPos;
    v_prevPos = prevProjectionViewReverseZ * prevWorldPos;

    gl_Position = rasterProjectionView * worldPos;

    v_uv = a_uv;
}
