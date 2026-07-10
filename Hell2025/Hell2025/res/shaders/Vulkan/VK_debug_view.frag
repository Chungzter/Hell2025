#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/normal_encoding.glsl"
#include "../common/util.glsl"
#include "../common/reconstruction.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/binding_indices.glsl"
#include "../common/Vulkan/push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_UINT_TEXTURES) uniform utexture2D uintTextures[];

layout(location = 0) out vec4 out_color;

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer { ViewportData viewportDataArr[]; };
layout(buffer_reference, scalar) readonly buffer RendererDataBuffer { RendererData rendererData; };

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebugView data;
} pushConstant;

#define OVERRIDE_NONE 0
#define OVERRIDE_BASE_COLOR 1
#define OVERRIDE_NORMALS 2
#define OVERRIDE_RMA 3
#define OVERRIDE_ROUGHNESS 4
#define OVERRIDE_METALLIC 5
#define OVERRIDE_AO 6
#define OVERRIDE_CAMERA_NDOTL 7
#define OVERRIDE_TILE_HEATMAP_LIGHTS 8
#define OVERRIDE_TILE_HEATMAP_BLOOD_DECALS 9
#define OVERRIDE_TILE_HEATMAP_CHRISTMAS_LIGHTS 10
#define OVERRIDE_INDIRECT_DIFFUSE 11
#define OVERRIDE_VELOCITY 12
#define OVERRIDE_VIS_BUFFER 13
#define OVERRIDE_DEPTH 14
#define OVERRIDE_WORLD_POSITION 15
#define OVERRIDE_EMISSIVE 16

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
    return DecodeNormal(GetNormalXYRoughnessMisc(px).rg);
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

    vec3 normal = DecodeNormal(GetNormalXYRoughnessMisc(px).rg);
    vec3 lightDir = normalize(viewportData.inverseView[2].xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);

    return GetBaseColor(px) * ndotl;
}

vec3 GetVelocity(ivec2 px) {
    vec2 velocity = GetVelocityXYOcclusionSubSurface(px).rg;
    return vec3(velocity * 20.0 + 0.5, 0.5);
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
    else if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_DIFFUSE ||
             rendererData.rendererOverrideState == OVERRIDE_EMISSIVE ||
             rendererData.rendererOverrideState == OVERRIDE_TILE_HEATMAP_LIGHTS ||
             rendererData.rendererOverrideState == OVERRIDE_TILE_HEATMAP_BLOOD_DECALS ||
             rendererData.rendererOverrideState == OVERRIDE_TILE_HEATMAP_CHRISTMAS_LIGHTS) {
        finalColor = vec3(1.0, 0.0, 1.0);
    }

    out_color = vec4(finalColor, 1.0);
}
