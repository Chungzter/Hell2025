#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/normal_encoding.glsl"
#include "../common/util.glsl"
#include "../common/reconstruction.glsl"
#include "../common/viewport.glsl"
#include "../common/flags.glsl"
#include "../common/Vulkan/binding_indices.glsl"
#include "../common/Vulkan/push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 0, binding = DESC_IDX_ACCELERATION_STRUCTURES) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

layout(location = 0) out vec4 out_color;

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer { ViewportData viewportDataArr[]; };
layout(buffer_reference, scalar) readonly buffer RendererDataBuffer { RendererData rendererData; };
layout(buffer_reference, scalar) readonly buffer LightBuffer        { Light lights[]; };
layout(buffer_reference, scalar) readonly buffer MaterialBuffer     { Material materials[]; };

struct PackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

struct RayQueryInstanceData {
    uint geometryDataOffset;
    uint geometryDataCount;
    uint padding0;
    uint padding1;
};

struct RayQueryGeometryData {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint baseVertex;
    uint baseIndex;
    uint vertexCount;
    uint indexCount;
    uint blendingMode;
    int materialIndex;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    PackedVertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uint indices[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryInstanceDataBuffer {
    RayQueryInstanceData instanceData[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryGeometryDataBuffer {
    RayQueryGeometryData geometryData[];
};

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDeferredLighting data;
} pushConstant;


float RayQueryLineOfSight(vec3 rayOrigin, vec3 target) {
    vec3 rayVector = target - rayOrigin;
    float rayLength = length(rayVector);

    const float rayTMin = 0.001;
    const float targetBias = 0.01;

    float rayTMax = rayLength - targetBias;
    if (rayTMax <= rayTMin) {
        return 1.0;
    }

    vec3 rayDirection = rayVector / rayLength;

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, 0u, 0xff, rayOrigin, rayTMin, rayDirection, rayTMax);

    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        RayQueryInstanceDataBuffer rayQueryInstanceDataBuffer = RayQueryInstanceDataBuffer(pushConstant.data.rayQueryInstanceDataDeviceAddress);
        RayQueryGeometryDataBuffer rayQueryGeometryDataBuffer = RayQueryGeometryDataBuffer(pushConstant.data.rayQueryGeometryDataDeviceAddress);

        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryInstanceData instanceData = rayQueryInstanceDataBuffer.instanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= instanceData.geometryDataCount) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return 0.0;
        }

        RayQueryGeometryData geometryData = rayQueryGeometryDataBuffer.geometryData[instanceData.geometryDataOffset + geometryIndex];
        bool alphaDiscard = geometryData.blendingMode == BLENDING_MODE_ALPHA_DISCARD && geometryData.materialIndex >= 0;

        if (alphaDiscard) {
            MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.materialsDeviceAddress);
            Material material = materialBuffer.materials[geometryData.materialIndex];
            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            uint localIndexOffset = primitiveIndex * 3u;
            if (localIndexOffset + 2u >= geometryData.indexCount) {
                rayQueryConfirmIntersectionEXT(rayQuery);
                return 0.0;
            }

            IndexBuffer indexBuffer = IndexBuffer(geometryData.indexBufferDeviceAddress);
            uint i0 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 0u] + geometryData.baseVertex;
            uint i1 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 1u] + geometryData.baseVertex;
            uint i2 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 2u] + geometryData.baseVertex;
            uint vertexEnd = geometryData.baseVertex + geometryData.vertexCount;
            if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
                rayQueryConfirmIntersectionEXT(rayQuery);
                return 0.0;
            }

            VertexBuffer vertexBuffer = VertexBuffer(geometryData.vertexBufferDeviceAddress);
            PackedVertex v0 = vertexBuffer.vertices[i0];
            PackedVertex v1 = vertexBuffer.vertices[i1];
            PackedVertex v2 = vertexBuffer.vertices[i2];

            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
            vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
            vec2 uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

            uint textureIndex = uint(material.basecolor);
            float alpha = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0).a;
            if (alpha < 0.25) {
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
        return 0.0;
    }

    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}
void main() {
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.viewportDataDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.rendererDataDeviceAddress);
    LightBuffer lightBuffer = LightBuffer(pushConstant.data.lightsDeviceAddress);

    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);

    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, outputImageSize, rendererDataBuffer.rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];

    vec4 baseColorMetallic = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    vec4 normalXYRoughnessMisc = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    vec3 normal = DecodeNormal(normalXYRoughnessMisc.rg);
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);
    vec3 worldPos = WorldPosFromDepth_VK(viewportUV, depth, viewportData.inverseProjectionViewReverseZ);
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 10; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 toLight = lightPosition - worldPos;
        float dist = length(toLight);
        vec3 lightDir = toLight / max(dist, 0.0001);
        float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
        float ndotl = max(dot(normal, lightDir), 0.0);
        if (ndotl <= 0.0 || attenuation <= 0.0) {
            continue;
        }

        vec3 rayOrigin = worldPos + normal * 0.001;
        vec3 target = lightPosition;

        float visibility = pushConstant.data.rayQueryEnabled != 0u ? RayQueryLineOfSight(rayOrigin, target) : 1.0;
        lighting += visibility * ndotl * attenuation * clamp(vec3(light.colorR, light.colorG, light.colorB), 0.0, 1.0);
    }

    out_color = vec4(baseColorMetallic.rgb * lighting, 1.0);
}
