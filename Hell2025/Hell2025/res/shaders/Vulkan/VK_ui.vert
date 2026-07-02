#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/Vulkan/types.glsl"
#include "../common/Vulkan/push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsUI data;
} pushConstant;

layout(buffer_reference, scalar) readonly buffer RenderItemUIBuffer {
    RenderItemUI data[];
};

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) flat out uint v_textureIndex;
layout(location = 3) flat out uint v_filterMode;

void main() {
    RenderItemUIBuffer renderItems = RenderItemUIBuffer(pushConstant.data.renderItemsDeviceAddress);
    RenderItemUI renderItem = renderItems.data[gl_InstanceIndex];

    v_uv = a_uv;
    v_color = a_color;
    v_textureIndex = renderItem.textureIndex;
    v_filterMode = renderItem.filterMode;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
