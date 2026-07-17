#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebug3D data;
} pushConstant;

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData data[];
};

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in ivec2 a_pixelOffset;
layout(location = 3) in int a_depthEnabled;
layout(location = 4) in int a_exclusiveViewportIndex;
layout(location = 5) in int a_ignoredViewportIndex;

layout(location = 0) out vec3 v_color;

void main() {
    ViewportDataBuffer viewportData = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);
    mat4 projectionView = viewportData.data[pushConstant.data.viewportIndex].projectionViewReverseZ;

    v_color = a_color;
    gl_Position = projectionView * vec4(a_position, 1.0);
    gl_PointSize = 8.0;

    if (a_exclusiveViewportIndex != -1 && a_exclusiveViewportIndex != int(pushConstant.data.viewportIndex)) {
        gl_Position = vec4(0, 0, 0, 0);
    }
    if (a_ignoredViewportIndex != -1 && a_ignoredViewportIndex == int(pushConstant.data.viewportIndex)) {
        gl_Position = vec4(0, 0, 0, 0);
    }
    if (a_depthEnabled != 1) {
        gl_Position.z = 0.99999 * gl_Position.w;
    }
}
