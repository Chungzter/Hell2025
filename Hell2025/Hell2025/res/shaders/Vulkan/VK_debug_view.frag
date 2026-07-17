#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/normal_encoding.glsl"
#include "../common/post_processing.glsl"
#include "../common/util.glsl"
#include "../common/reconstruction.glsl"
#include "../common/renderer_override_modes.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_UINT_TEXTURES) uniform utexture2D uintTextures[];

layout(location = 0) out vec4 out_color;

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer { ViewportData viewportDataArr[]; };
layout(buffer_reference, scalar) readonly buffer RendererDataBuffer { RendererData rendererData; };

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebugView data;
} pushConstant;

vec3 IntegerToColor(uint x) {
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return vec3(float(x & 0xffu), float((x >> 8) & 0xffu), float((x >> 16) & 0xffu)) / 255.0;
}

vec4 GetBaseColorMetallic(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec4 GetNormalXYRoughnessMisc(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec4 GetVelocityXYOcclusionSubSurface(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_VELOCITY_XY_OCCLUSION_SUBSURFACE], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec3 GetBaseColor(ivec2 px) {
    return GetBaseColorMetallic(px).rgb;
}

vec3 GetNormal(ivec2 px) {
    return DecodeOct(GetNormalXYRoughnessMisc(px).rg);
}

vec3 GetRoughness(ivec2 px) {
    return vec3(GetNormalXYRoughnessMisc(px).b);
}

vec3 GetMetallic(ivec2 px) {
    return vec3(GetBaseColorMetallic(px).a);
}

vec3 GetAO(ivec2 px) {
    return vec3(GetVelocityXYOcclusionSubSurface(px).b);
}

vec3 GetRMA(ivec2 px) {
    return vec3(GetRoughness(px).r, GetMetallic(px).r, GetAO(px).r);
}

vec3 GetCameraNdotL(ivec2 px, ivec2 outputImageSize, RendererData rendererData, ViewportDataBuffer viewportDataBuffer) {
    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, outputImageSize, rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];

    vec3 normal = DecodeOct(GetNormalXYRoughnessMisc(px).rg);
    vec3 lightDir = normalize(viewportData.inverseView[2].xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);

    return GetBaseColor(px) * ndotl;
}

vec3 GetVelocity(ivec2 px) {
    vec2 velocity = GetVelocityXYOcclusionSubSurface(px).rg;
    return vec3(velocity * 20.0 + 0.5, 0.5);
}

vec3 GetIndirectDiffuse(ivec2 px, ivec2 outputImageSize) {
    vec2 screenUV = (vec2(px) + 0.5) / vec2(outputImageSize);
    vec3 indirectDiffuseTexture = texture(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE], samplers[VULKAN_SAMPLER_IDX_LINEAR]), screenUV).rgb;
    vec3 result = Tonemap_ACES(indirectDiffuseTexture);
    result = pow(result, vec3(1.0 / 2.2));
    result = clamp(result, 0.0, 1.0);
    result = mix(result, Tonemap_ACES(result), 0.125);
    return result;
}

vec3 GetVisBuffer(ivec2 px) {
    uvec2 visibilityData = texelFetch(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).rg;
    uint meshID = visibilityData.x;
    uint primitiveID = visibilityData.y;

    if (meshID == 0u && primitiveID == 0u) {
        return vec3(0.05);
    }

    vec3 meshColor = IntegerToColor(meshID);
    vec3 primitiveColor = IntegerToColor(primitiveID);
    return mix(meshColor, primitiveColor, 0.2);
}

vec3 GetDepth(ivec2 px) {
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    return vec3(depth, 0.0, 0.0);
}

vec3 GetWorldPosition(ivec2 px, ivec2 outputImageSize, RendererData rendererData, ViewportDataBuffer viewportDataBuffer) {
    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, outputImageSize, rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    return WorldPosFromDepth_VK(viewportUV, depth, viewportData.inverseProjectionViewReverseZ);
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);

    if (px.x < 0 || px.y < 0 || px.x >= outputImageSize.x || px.y >= outputImageSize.y) {
        discard;
    }

    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.frame.rendererDataDeviceAddress);
    RendererData rendererData = rendererDataBuffer.rendererData;

    vec3 finalColor = vec3(0.0);

    if (rendererData.rendererOverrideState == OVERRIDE_BASE_COLOR) {
        finalColor = GetBaseColor(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_NORMALS) {
        finalColor = GetNormal(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_RMA) {
        finalColor = GetRMA(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_ROUGHNESS) {
        finalColor = GetRoughness(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_METALLIC) {
        finalColor = GetMetallic(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_AO) {
        finalColor = GetAO(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_CAMERA_NDOTL) {
        finalColor = GetCameraNdotL(px, outputImageSize, rendererData, viewportDataBuffer);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_VELOCITY) {
        finalColor = GetVelocity(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_VIS_BUFFER) {
        finalColor = GetVisBuffer(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_DEPTH) {
        finalColor = GetDepth(px);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_WORLD_POSITION) {
        finalColor = GetWorldPosition(px, outputImageSize, rendererData, viewportDataBuffer);
    }
    else if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_DIFFUSE) {
        finalColor = GetIndirectDiffuse(px, outputImageSize);
    }

    out_color = vec4(finalColor, 1.0);
}
