#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/lighting.glsl"
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
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

layout(early_fragment_tests) in;
layout(location = 0) out vec4 out_color;

#define LIGHT_COUNT 9             // TODO "tile based deferred" me the fuck outta here
#define ALPHA_TEST_THRESHOLD 0.25 // TODO: Make me instance specific, which means adding it to the RenderItem struct

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer { ViewportData viewportDataArr[]; };
layout(buffer_reference, scalar) readonly buffer RendererDataBuffer { RendererData rendererData; };
layout(buffer_reference, scalar) readonly buffer LightBuffer        { Light lights[]; };
layout(buffer_reference, scalar) readonly buffer MaterialBuffer     { Material materials[]; };

// Must match the ray query vertex buffer layout
struct PackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

// TLAS instanceCustomIndex points here
struct RayQueryInstanceData {
    uint geometryDataOffset;
    uint geometryDataCount;
    uint padding0;
    uint padding1;
};

// One entry per mesh range inside the hit BLAS
struct RayQueryGeometryData {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint baseVertex;
    uint baseIndex;
    uint vertexCount;
    uint indexCount;
    uint blendingMode;
    int materialIndex;
    uint shadowBit;
    uint padding0;
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

//vec3 FresnelSchlick(float cosTheta, vec3 f0) {
//    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
//}

float MaxComponent(vec3 v) {
    return max(max(v.x, v.y), v.z);
}

float Hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 Hash22(vec2 p) {
    float x = Hash12(p);
    float y = Hash12(p + 17.17);
    return vec2(x, y);
}

vec3 GetJitterRay(vec3 dir, float lightSize, float sampleIndex) {
    vec3 n = normalize(dir);

    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);

    vec2 seed = gl_FragCoord.xy + n.xy * 113.1 + vec2(sampleIndex * 37.17, sampleIndex * 91.73);
    vec2 r = Hash22(seed);
    float angle = r.x * 6.28318530718;
    float radius = sqrt(r.y) * lightSize;

    vec2 disk = vec2(cos(angle), sin(angle)) * radius;

    return normalize(n + tangent * disk.x + bitangent * disk.y);
}

float GetSingleRaySpecularTraceWeight(vec3 fresnel, float roughness) {
    if (MaxComponent(fresnel) < 0.02 || roughness >= 0.85) {
        return 0.0;
    }

    // Without a denoiser, wide glossy lobes are better handled by probes/IBL later.
    return 1.0 - smoothstep(0.35, 0.85, roughness);
}

vec3 ImportanceSampleGGX(vec2 xi, vec3 normal, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));

    vec3 halfVector = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(tangent * halfVector.x + bitangent * halfVector.y + normal * halfVector.z);
}

bool GetIndirectSpecularRaySample(vec3 normal, vec3 viewDir, vec3 linearBaseColor, float roughness, float metallic, out vec3 rayDirection, out vec3 rayWeight) {
    rayDirection = vec3(0.0);
    rayWeight = vec3(0.0);

    float noV = clamp(dot(normal, viewDir), 0.0, 1.0);
    if (noV <= 0.0) {
        return false;
    }

    vec3 f0 = mix(vec3(0.04), linearBaseColor, metallic);
    vec3 fresnel = FresnelSchlick(noV, f0);
    float traceWeight = GetSingleRaySpecularTraceWeight(fresnel, roughness);
    if (traceWeight <= 0.0) {
        return false;
    }

    if (roughness < 0.03) {
        rayDirection = normalize(reflect(-viewDir, normal));
        rayWeight = fresnel * traceWeight;
        return dot(normal, rayDirection) > 0.0;
    }

    vec2 xi = Hash22(gl_FragCoord.xy + normal.xy * 173.13 + vec2(roughness * 41.7, metallic * 89.1));
    vec3 halfVector = ImportanceSampleGGX(xi, normal, max(roughness, 0.03));
    rayDirection = normalize(reflect(-viewDir, halfVector));

    float noL = clamp(dot(normal, rayDirection), 0.0, 1.0);
    float noH = clamp(dot(normal, halfVector), 0.0, 1.0);
    float voH = clamp(dot(viewDir, halfVector), 0.0, 1.0);
    if (noL <= 0.0 || noH <= 0.0 || voH <= 0.0) {
        return false;
    }

    float pdf = DistributionGGX(normal, halfVector, roughness) * noH / max(4.0 * voH, 0.0001);
    if (pdf <= 0.0001) {
        return false;
    }

    vec3 specularBRDF = microfacetBRDFSpecularOnly(rayDirection, viewDir, normal, linearBaseColor, metallic, 1.0, roughness);
    rayWeight = specularBRDF * noL / pdf * traceWeight;
    return MaxComponent(rayWeight) > 0.001;
}

vec3 GetDirectLightingForRayHit(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 normal, vec3 worldPos, vec3 linearBaseColor, float roughness, float metallic, vec3 receiverWorldPos) {
    vec3 toLight = lightPos - worldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 0.0001);
    vec3 viewDir = normalize(receiverWorldPos - worldPos);
    float attenuation = smoothstep(radius, 0.0, dist) * strength;
    float ndotl = max(dot(normal, lightDir), 0.0);

    if (ndotl <= 0.0 || attenuation <= 0.0) {
        return vec3(0.0);
    }

    vec3 brdf = microfacetBRDF(lightDir, viewDir, normal, linearBaseColor, metallic, 1.0, roughness);
    return brdf * ndotl * attenuation * clamp(lightColor, 0.0, 1.0);
}

bool SampleRayQueryBaseColor(RayQueryGeometryData geometryData, uint primitiveIndex, vec2 barycentrics, out vec4 baseColor) {
    // Rebuild triangle UV from primitive index and barycentrics
    baseColor = vec4(0.0);

    if (geometryData.materialIndex < 0) {
        return false;
    }

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= geometryData.indexCount) {
        return false;
    }

    IndexBuffer indexBuffer = IndexBuffer(geometryData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 0u] + geometryData.baseVertex;
    uint i1 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 1u] + geometryData.baseVertex;
    uint i2 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 2u] + geometryData.baseVertex;
    uint vertexEnd = geometryData.baseVertex + geometryData.vertexCount;
    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return false;
    }

    VertexBuffer vertexBuffer = VertexBuffer(geometryData.vertexBufferDeviceAddress);
    PackedVertex v0 = vertexBuffer.vertices[i0];
    PackedVertex v1 = vertexBuffer.vertices[i1];
    PackedVertex v2 = vertexBuffer.vertices[i2];

    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec2 uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

    // Ray query stores material, material stores texture
    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[geometryData.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    uint textureIndex = uint(material.basecolor);
    baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0);
    return true;
}

bool SampleRayQuerySurface(RayQueryGeometryData geometryData, uint primitiveIndex, vec2 barycentrics, mat4x3 objectToWorld, vec3 rayDirection, out vec4 baseColor, out float roughness, out float metallic, out vec3 worldPos, out vec3 worldNormal) {
    baseColor = vec4(0.0);
    roughness = 1.0;
    metallic = 0.0;
    worldPos = vec3(0.0);
    worldNormal = vec3(0.0, 1.0, 0.0);

    if (geometryData.materialIndex < 0) {
        return false;
    }

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= geometryData.indexCount) {
        return false;
    }

    IndexBuffer indexBuffer = IndexBuffer(geometryData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 0u] + geometryData.baseVertex;
    uint i1 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 1u] + geometryData.baseVertex;
    uint i2 = indexBuffer.indices[geometryData.baseIndex + localIndexOffset + 2u] + geometryData.baseVertex;
    uint vertexEnd = geometryData.baseVertex + geometryData.vertexCount;
    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return false;
    }

    VertexBuffer vertexBuffer = VertexBuffer(geometryData.vertexBufferDeviceAddress);
    PackedVertex v0 = vertexBuffer.vertices[i0];
    PackedVertex v1 = vertexBuffer.vertices[i1];
    PackedVertex v2 = vertexBuffer.vertices[i2];

    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec2 uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[geometryData.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    uint textureIndex = uint(material.basecolor);
    baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0);

    if (material.rma >= 0) {
        uint rmaTextureIndex = uint(material.rma);
        vec4 rma = textureLod(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), uv, 0.0);
        roughness = rma.r;
        metallic = rma.g;
    }

    vec3 objectPos =
        vec3(v0.vx, v0.vy, v0.vz) * weights.x +
        vec3(v1.vx, v1.vy, v1.vz) * weights.y +
        vec3(v2.vx, v2.vy, v2.vz) * weights.z;

    vec3 objectNormal = normalize(
        vec3(v0.nx, v0.ny, v0.nz) * weights.x +
        vec3(v1.nx, v1.ny, v1.nz) * weights.y +
        vec3(v2.nx, v2.ny, v2.nz) * weights.z);

    vec3 objectTangent = normalize(
        vec3(v0.tx, v0.ty, v0.tz) * weights.x +
        vec3(v1.tx, v1.ty, v1.tz) * weights.y +
        vec3(v2.tx, v2.ty, v2.tz) * weights.z);

    worldPos = objectToWorld * vec4(objectPos, 1.0);
    worldNormal = normalize(objectToWorld * vec4(objectNormal, 0.0));
    vec3 worldTangent = normalize(objectToWorld * vec4(objectTangent, 0.0));
    worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);
    vec3 worldBitangent = cross(worldNormal, worldTangent);

    if (material.normal >= 0) {
        uint normalTextureIndex = uint(material.normal);
        vec3 normalMap = textureLod(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), uv, 0.0).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        worldNormal = normalize(mat3(worldTangent, worldBitangent, worldNormal) * normalMap);
    }

    if (dot(worldNormal, rayDirection) > 0.0) {
        worldNormal = -worldNormal;
    }

    return true;
}

float TraceShadowRay(vec3 rayOrigin, vec3 rayDir, float rayTMin, float rayTMax) {
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, 0u, 0xff, rayOrigin, rayTMin, rayDir, rayTMax);

    // Walk candidates until a blocker is accepted or the BVH ends
    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        RayQueryInstanceDataBuffer rayQueryInstanceDataBuffer = RayQueryInstanceDataBuffer(pushConstant.data.rayQueryInstanceDataDeviceAddress);
        RayQueryGeometryDataBuffer rayQueryGeometryDataBuffer = RayQueryGeometryDataBuffer(pushConstant.data.rayQueryGeometryDataDeviceAddress);

        // instanceCustomIndex points to the geometry range table
        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryInstanceData instanceData = rayQueryInstanceDataBuffer.instanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= instanceData.geometryDataCount) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return 0.0;
        }

        RayQueryGeometryData geometryData = rayQueryGeometryDataBuffer.geometryData[instanceData.geometryDataOffset + geometryIndex];

        // Skip non shadow casting geometry
        if ((geometryData.shadowBit & SHADOW_FLAG_POINT_LIGHT) == 0u) {
            continue;
        }

        // Skip blended geometry (aka eyebrows)
        if (geometryData.blendingMode == BLENDING_MODE_BLENDED) {
            continue;
        }

        // Skip any alpha tested geometry where sampled hit pixel is transparent
        if (geometryData.blendingMode == BLENDING_MODE_ALPHA_DISCARD ||
            geometryData.blendingMode == BLENDING_MODE_HAIR ||
            geometryData.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {

            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            vec4 hitBaseColor;
            if (SampleRayQueryBaseColor(geometryData, primitiveIndex, barycentrics, hitBaseColor) &&
                hitBaseColor.a < ALPHA_TEST_THRESHOLD) {
                continue;
            }
        }

        // This hit blocks the light
        rayQueryConfirmIntersectionEXT(rayQuery);
        return 0.0;
    }

    // No committed hit means clear line of sight
    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

float GetShadowVisibility(vec3 rayOrigin, vec3 target) {
    vec3 rayVector = target - rayOrigin;
    float rayLength = length(rayVector);

    const float rayTMin = 0.001;
    const float targetBias = 0.01;

    float rayTMax = rayLength - targetBias;
    if (rayTMax <= rayTMin) {
        return 1.0;
    }

    vec3 rayDir = rayVector / rayLength;

    const int shadowSampleCount = 1;
    const float shadowLightSize = 0.0;

    float visibility = 0.0;
    for (int i = 0; i < shadowSampleCount; i++) {
        vec3 jitteredRayDir = GetJitterRay(rayDir, shadowLightSize, float(i));
        visibility += TraceShadowRay(rayOrigin, jitteredRayDir, rayTMin, rayTMax);
    }

    return visibility / float(shadowSampleCount);
}

//vec3 DirectLighting(vec3 worldPos, vec3 normal, vec3 baseColor) {
//    LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);
//    vec3 lighting = vec3(0.0);
//
//    // Hardcoded light loop until tiled deferred is back
//    for (int i = 0; i < LIGHT_COUNT; i++) {
//        Light light = lightBuffer.lights[i];
//        if (light.radius <= 0.0 || light.strength <= 0.0) {
//            continue;
//        }
//
//        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
//        vec3 toLight = lightPosition - worldPos;
//        float dist = length(toLight);
//        vec3 lightDir = toLight / max(dist, 0.0001);
//        float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
//        float ndotl = max(dot(normal, lightDir), 0.0);
//        if (ndotl <= 0.0 || attenuation <= 0.0) {
//            continue;
//        }
//
//        float visibility = GetShadowVisibility(worldPos + normal * 0.001, lightPosition);
//        lighting += visibility * ndotl * attenuation * clamp(vec3(light.colorR, light.colorG, light.colorB), 0.0, 1.0);
//    }
//
//    return lighting * baseColor;
//}

bool RayQueryReflectedSurface(vec3 rayOrigin, vec3 rayDirection, out vec3 reflectedBaseColor, out float reflectedRoughness, out float reflectedMetallic, out vec3 reflectedWorldPos, out vec3 reflectedNormal) {
    // Finds first non-mirror surface in the reflection ray.
    reflectedBaseColor = vec3(0.0);
    reflectedRoughness = 1.0;
    reflectedMetallic = 0.0;
    reflectedWorldPos = vec3(0.0);
    reflectedNormal = vec3(0.0, 1.0, 0.0);

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, 0u, 0xff, rayOrigin, 0.01, rayDirection, 80.0);

    // Skip unusable candidates until one should be committed
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
            continue;
        }

        RayQueryGeometryData geometryData = rayQueryGeometryDataBuffer.geometryData[instanceData.geometryDataOffset + geometryIndex];
        if (geometryData.blendingMode == BLENDING_MODE_MIRROR) {
            // Mirrors should not reflect themselves forever
            continue;
        }

        uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
        vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

        vec4 hitBaseColor;
        if (!SampleRayQueryBaseColor(geometryData, primitiveIndex, barycentrics, hitBaseColor)) {
            continue;
        }

        if (geometryData.blendingMode == BLENDING_MODE_ALPHA_DISCARD ||
            geometryData.blendingMode == BLENDING_MODE_HAIR ||
            geometryData.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {

            if (hitBaseColor.a < ALPHA_TEST_THRESHOLD) {
                // Transparent pixels are invisible to reflections
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
    }

    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    if (intersectionType == gl_RayQueryCommittedIntersectionNoneEXT) {
        return false;
    }

    RayQueryInstanceDataBuffer rayQueryInstanceDataBuffer = RayQueryInstanceDataBuffer(pushConstant.data.rayQueryInstanceDataDeviceAddress);
    RayQueryGeometryDataBuffer rayQueryGeometryDataBuffer = RayQueryGeometryDataBuffer(pushConstant.data.rayQueryGeometryDataDeviceAddress);

    uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
    RayQueryInstanceData instanceData = rayQueryInstanceDataBuffer.instanceData[instanceIndex];

    uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
    if (geometryIndex >= instanceData.geometryDataCount) {
        return false;
    }

    RayQueryGeometryData geometryData = rayQueryGeometryDataBuffer.geometryData[instanceData.geometryDataOffset + geometryIndex];
    uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);

    vec4 hitBaseColor;
    if (!SampleRayQuerySurface(geometryData, primitiveIndex, barycentrics, objectToWorld, rayDirection, hitBaseColor, reflectedRoughness, reflectedMetallic, reflectedWorldPos, reflectedNormal)) {
        return false;
    }

    reflectedBaseColor = hitBaseColor.rgb;
    return true;
}

bool TraceRaytracedRadiance(vec3 rayOrigin, vec3 rayDirection, vec3 receiverWorldPos, out vec3 radiance) {
    radiance = vec3(0.0);

    vec3 reflectedBaseColor = vec3(0.0);
    float reflectedRoughness = 1.0;
    float reflectedMetallic = 0.0;
    vec3 reflectedWorldPos = vec3(0.0);
    vec3 reflectedNormal = vec3(0.0, 1.0, 0.0);

    if (!RayQueryReflectedSurface(rayOrigin, rayDirection, reflectedBaseColor, reflectedRoughness, reflectedMetallic, reflectedWorldPos, reflectedNormal)) {
        return false;
    }

    vec3 reflectedLinearBaseColor = pow(reflectedBaseColor, vec3(2.2));

    LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);
    for (int i = 0; i < LIGHT_COUNT; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
        float visibility = GetShadowVisibility(reflectedWorldPos + reflectedNormal * 0.001, lightPosition);

        radiance += GetDirectLightingForRayHit(
            lightPosition,
            lightColor,
            light.radius,
            light.strength,
            reflectedNormal,
            reflectedWorldPos,
            reflectedLinearBaseColor,
            reflectedRoughness,
            reflectedMetallic,
            receiverWorldPos) * visibility;
    }

    radiance += reflectedLinearBaseColor * 0.01;
    return true;
}

void main() {
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.frame.rendererDataDeviceAddress);

    // Get viewport data
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, outputImageSize, rendererDataBuffer.rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];
    vec3 viewPos = viewportData.viewPos.xyz;

    // Fetch GBuffer
    vec4 baseColorMetallic = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    vec4 normalXYRoughnessMisc = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    // Reconstruct position from depth
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);
    vec3 worldPos = WorldPosFromDepth_VK(viewportUV, depth, viewportData.inverseProjectionViewReverseZ);

    // Reconstruct materials
    vec3 baseColor = baseColorMetallic.rgb;
    vec3 normal = DecodeNormal(normalXYRoughnessMisc.rg);
    float metallic = baseColorMetallic.a;
    float roughness = normalXYRoughnessMisc.b;

    vec3 linearBaseColor = pow(baseColor, vec3(2.2));

    // Decode flags
    uint miscFlags = DecodeMiscFlags(normalXYRoughnessMisc.a);
    bool isMirrorSurface = (miscFlags & MISC_FLAG_MIRROR_SURFACE) != 0u;

    // Direct light
    vec3 directLighting = vec3(0.0);

    if (!isMirrorSurface) {

        LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);
        //vec3 lighting = vec3(0.0);

        // Hardcoded light loop until tiled deferred is back
        for (int i = 0; i < LIGHT_COUNT; i++) {
            Light light = lightBuffer.lights[i];
            if (light.radius <= 0.0 || light.strength <= 0.0) {
                continue;
            }

            vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
            vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);

            vec3 toLight = lightPosition - worldPos;
            float dist = length(toLight);
            vec3 lightDir = toLight / max(dist, 0.0001);
            float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
            float ndotl = max(dot(normal, lightDir), 0.0);
            if (ndotl <= 0.0 || attenuation <= 0.0) {
                continue;
            }

            float visibility = GetShadowVisibility(worldPos + normal * 0.001, lightPosition);


            vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * visibility;


            directLighting += directLight;
        }
    }

    // Indirect specular
    vec3 indirectSpecular = vec3(0.0);

    if (!isMirrorSurface) {
        vec3 viewDirToCamera = normalize(viewPos - worldPos);
        vec3 specularRayDirection = vec3(0.0);
        vec3 specularRayWeight = vec3(0.0);

        if (GetIndirectSpecularRaySample(normal, viewDirToCamera, linearBaseColor, roughness, metallic, specularRayDirection, specularRayWeight)) {
            vec3 reflectedRadiance = vec3(0.0);

            if (TraceRaytracedRadiance(worldPos + normal * 0.01, specularRayDirection, worldPos, reflectedRadiance)) {
                indirectSpecular = reflectedRadiance * specularRayWeight;
            }
        }
    }

    // Mirror
    vec3 mirrorLighting = vec3(0.0);

    if (isMirrorSurface) {
        vec3 viewDirToCamera = normalize(viewPos - worldPos);
        vec3 reflectionDir = normalize(reflect(-viewDirToCamera, normal));
        vec3 reflectedRadiance = vec3(0.0);

        if (TraceRaytracedRadiance(worldPos + normal * 0.01, reflectionDir, worldPos, reflectedRadiance)) {
            mirrorLighting = reflectedRadiance;
        }
    }

    vec3 ambient = linearBaseColor * 0.01;

    // Final composite
    vec3 finalLighting = directLighting + indirectSpecular + mirrorLighting + ambient;

    finalLighting += baseColor.rgb * 0.0025;

    out_color = vec4(finalLighting, 1.0);
}
