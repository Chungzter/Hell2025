#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsPointShadow data;
} pc;

layout(buffer_reference, scalar) readonly buffer PointShadowFaceDataBuffer {
    PointShadowFaceData faceData[];
};

layout(location = 0) in vec3 a_position;
layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) flat out vec4 v_lightPositionRadius;

void main() {
    RenderItemBuffer renderItems = pc.data.frame.renderItemBuffer;
    uint baseInstance = uint(gl_BaseInstanceARB);
    uint faceDataIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
    uint instanceOffset = baseInstance & uint((1 << VIEWPORT_INDEX_SHIFT) - 1);
    uint globalInstanceIndex = instanceOffset + (uint(gl_InstanceIndex) - baseInstance);
    RenderItem renderItem = renderItems.renderItems[globalInstanceIndex];
    PointShadowFaceDataBuffer faceDataBuffer = PointShadowFaceDataBuffer(pc.data.faceDataDeviceAddress);
    PointShadowFaceData faceData = faceDataBuffer.faceData[faceDataIndex];
    vec4 worldPosition = renderItem.modelMatrix * vec4(a_position, 1.0);

    v_worldPosition = worldPosition.xyz;
    v_lightPositionRadius = faceData.lightPositionRadius;
    gl_Position = faceData.projectionView * worldPosition;
    gl_Layer = int(faceData.arrayLayer);
}
