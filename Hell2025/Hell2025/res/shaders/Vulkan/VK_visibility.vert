#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsVisibility data;
} pushConstant;

layout(buffer_reference, scalar) readonly buffer RenderItemBuffer {
    RenderItem data[];
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material data[];
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData data[];
};

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) flat out uint v_globalInstanceIndex;
layout(location = 1) out vec2 v_uv;
layout(location = 2) flat out int v_baseColorTextureIndex;

void main() {
    RenderItemBuffer renderItems = RenderItemBuffer(pushConstant.data.renderItemsDeviceAddress);
    MaterialBuffer materials = MaterialBuffer(pushConstant.data.materialsDeviceAddress);
    ViewportDataBuffer viewportData = ViewportDataBuffer(pushConstant.data.viewportDataDeviceAddress);

    uint baseInstance = uint(gl_BaseInstanceARB);
    uint viewportIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
    uint instanceOffset = baseInstance & uint((1 << VIEWPORT_INDEX_SHIFT) - 1);
    uint globalInstanceIndex = instanceOffset + (uint(gl_InstanceIndex) - baseInstance);

    RenderItem renderItem = renderItems.data[globalInstanceIndex];
    Material material = materials.data[renderItem.materialIndex];
    mat4 projectionView = viewportData.data[viewportIndex].projectionViewReverseZ;
    vec4 worldPos = renderItem.modelMatrix * vec4(a_position, 1.0);

    if (pushConstant.data.useDepthOffset != 0u) {
        vec3 cameraPos = viewportData.data[viewportIndex].viewPos.xyz;
        vec3 awayFromCamera = normalize(worldPos.xyz - cameraPos);
        float depthBiasMeters = 0.01;
        worldPos.xyz += awayFromCamera * depthBiasMeters;
    }

    v_globalInstanceIndex = globalInstanceIndex;
    v_uv = a_uv;
    v_baseColorTextureIndex = material.basecolor;
    gl_Position = projectionView * worldPos;
}
