/*#version 460 core

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

layout(std430, binding = 3) readonly restrict buffer viewportDataBuffer { ViewportData viewportData[]; };
layout(std430, binding = 1) readonly restrict buffer materialsBuffer    { Material materials[]; };
layout(std430, binding = 4) readonly buffer renderItemsBuffer           { RenderItem renderItems[]; };

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;

#if ENABLE_BINDLESS
out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;
out flat int WoundBaseColorTextureIndex;
out flat int WoundNormalTextureIndex;
out flat int WoundRMATextureIndex;
#else
uniform int u_viewportIndex;
uniform int u_globalInstanceIndex;
#endif

void main() {

#if ENABLE_BINDLESS
    int viewportIndex = gl_BaseInstance >> VIEWPORT_INDEX_SHIFT;
    int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    int globalInstanceIndex = instanceOffset + gl_InstanceID;

    Material material = materials[renderItems[globalInstanceIndex].materialIndex];
    BaseColorTextureIndex = material.basecolor;
	NormalTextureIndex = material.normal;
	RMATextureIndex = material.rma;

#else
    int globalInstanceIndex = u_globalInstanceIndex;
    int viewportIndex = u_viewportIndex;
#endif

    RenderItem renderItem = renderItems[globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
	mat4 projection = viewportData[viewportIndex].projection;
	mat4 view = viewportData[viewportIndex].view;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));

	TexCoord = vUV;
    ViewPos = viewportData[viewportIndex].inverseView[3].xyz;

    gl_Position = projection * view * WorldPos;

    // Camera relative position for depth precision
    //vec4 camRelativeWorldPos = vec4(WorldPos.xyz - ViewPos, 1.0);
    //gl_Position = projection * mat4(mat3(view)) * camRelativeWorldPos;

    //MiscFlags = MISC_FLAG_RESEVERED; // rethink this?
}
*/

#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform mat4 u_projectionView;
uniform mat4 u_inverseModel;
uniform mat4 u_model;

out vec2 TexCoord;
out vec4 v_worldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;

void main() {
    mat4 modelMatrix = u_model;
    mat4 inverseModelMatrix = u_inverseModel;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    TexCoord = vUV;

    v_worldPos = u_model * vec4(vPosition, 1.0);
    ViewPos = inverseModelMatrix[3].xyz;

	gl_Position = u_projectionView * v_worldPos;
}
