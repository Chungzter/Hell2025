#version 460 core

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform int u_viewportIndex;

out vec2 TexCoord;
out vec4 v_worldPos;
out vec3 v_normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;
out flat int MaterialIndex;
out flat uint v_instanceIndex;

readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

readonly restrict layout(std430, binding = SSBO_IDX_GLASS_INSTANCE_DATA) buffer glassRenderItemsBuffer {
    RenderItem glassRenderItems[];
};

void main() {

    int globalInstanceIndex = gl_BaseInstance + gl_InstanceID;
    RenderItem renderItem = glassRenderItems[globalInstanceIndex];

	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
	mat4 inverseView = viewportData[u_viewportIndex].inverseView;

    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = inverse(modelMatrix);
    mat4 normalMatrix = transpose(inverseModelMatrix);

    v_normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(v_normal, Tangent));
    TexCoord = vUV;
    MaterialIndex = renderItem.materialIndex;
    v_instanceIndex = uint(globalInstanceIndex);

    v_worldPos = modelMatrix * vec4(vPosition, 1.0);
    ViewPos = inverseView[3].xyz;

	gl_Position = projectionView * v_worldPos;
}
