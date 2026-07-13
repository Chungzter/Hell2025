#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/intersect.glsl"
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
struct RayQueryBLASInstanceData {
    uint meshInstanceDataOffset;
    uint meshInstanceDataCount;
    uint padding0;
    uint padding1;
};

struct RayQueryMesh {
    uint baseVertex;
    uint baseIndex;
    uint vertexCount;
    uint indexCount;
};

struct RayQueryMaterial {
    uint blendingMode;
    int materialIndex;
    uint shadowBit;
    uint padding0;
};

// One entry per mesh inside the hit BLAS
struct RayQueryMeshInstanceData {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    RayQueryMesh mesh;
    RayQueryMaterial material;
};

struct RayQuerySurfaceHit {
    bool hitFound;
    int materialIndex;
    vec2 uv;
    float rayT;
};

struct RayQueryTriangleSample {
    bool valid;
    PackedVertex v0;
    PackedVertex v1;
    PackedVertex v2;
    vec3 weights;
};

struct RayQueryReflectedSurfaceHit {
    RayQuerySurfaceHit surface;
    vec3 normal;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    PackedVertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uint indices[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryBLASInstanceDataBuffer {
    RayQueryBLASInstanceData blasInstanceData[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryMeshInstanceDataBuffer {
    RayQueryMeshInstanceData meshInstanceData[];
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

RayQuerySurfaceHit EmptyRayQuerySurfaceHit() {
    RayQuerySurfaceHit hit;
    hit.hitFound = false;
    hit.materialIndex = -1;
    hit.uv = vec2(0.0);
    hit.rayT = 0.0;
    return hit;
}

RayQueryTriangleSample EmptyRayQueryTriangleSample() {
    RayQueryTriangleSample triSample;
    triSample.valid = false;
    triSample.weights = vec3(0.0);
    return triSample;
}

RayQueryTriangleSample ResolveRayQueryTriangleSample(RayQueryMeshInstanceData meshInstanceData, uint primitiveIndex, vec2 barycentrics) {
    RayQueryTriangleSample triSample = EmptyRayQueryTriangleSample();

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= meshInstanceData.mesh.indexCount) {
        return triSample;
    }

    IndexBuffer indexBuffer = IndexBuffer(meshInstanceData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 0u] + meshInstanceData.mesh.baseVertex;
    uint i1 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 1u] + meshInstanceData.mesh.baseVertex;
    uint i2 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 2u] + meshInstanceData.mesh.baseVertex;
    uint vertexEnd = meshInstanceData.mesh.baseVertex + meshInstanceData.mesh.vertexCount;
    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return triSample;
    }

    VertexBuffer vertexBuffer = VertexBuffer(meshInstanceData.vertexBufferDeviceAddress);
    triSample.v0 = vertexBuffer.vertices[i0];
    triSample.v1 = vertexBuffer.vertices[i1];
    triSample.v2 = vertexBuffer.vertices[i2];
    triSample.weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    triSample.valid = true;
    return triSample;
}

RayQuerySurfaceHit ResolveRayQuerySurfaceHit(RayQueryMeshInstanceData meshInstanceData, RayQueryTriangleSample triSample, float rayT) {
    RayQuerySurfaceHit hit = EmptyRayQuerySurfaceHit();
    if (!triSample.valid || meshInstanceData.material.materialIndex < 0) {
        return hit;
    }

    hit.hitFound = true;
    hit.materialIndex = meshInstanceData.material.materialIndex;
    hit.uv =
        vec2(triSample.v0.u, triSample.v0.v) * triSample.weights.x +
        vec2(triSample.v1.u, triSample.v1.v) * triSample.weights.y +
        vec2(triSample.v2.u, triSample.v2.v) * triSample.weights.z;
    hit.rayT = rayT;
    return hit;
}

RayQuerySurfaceHit ResolveRayQuerySurfaceHit(RayQueryMeshInstanceData meshInstanceData, uint primitiveIndex, vec2 barycentrics, float rayT) {
    RayQueryTriangleSample triSample = ResolveRayQueryTriangleSample(meshInstanceData, primitiveIndex, barycentrics);
    return ResolveRayQuerySurfaceHit(meshInstanceData, triSample, rayT);
}

bool SampleRayHitBaseColor(RayQuerySurfaceHit hit, out vec4 baseColor) {
    baseColor = vec4(0.0);
    if (!hit.hitFound || hit.materialIndex < 0) {
        return false;
    }

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[hit.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    uint textureIndex = uint(material.basecolor);
    baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), hit.uv, 0.0);
    return true;
}

bool SampleRayHitMaterial(RayQuerySurfaceHit hit, out vec4 baseColor, out float roughness, out float metallic) {
    baseColor = vec4(0.0);
    roughness = 1.0;
    metallic = 0.0;

    if (!hit.hitFound || hit.materialIndex < 0) {
        return false;
    }

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[hit.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    uint textureIndex = uint(material.basecolor);
    baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), hit.uv, 0.0);

    if (material.rma >= 0) {
        uint rmaTextureIndex = uint(material.rma);
        vec4 rma = textureLod(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), hit.uv, 0.0);
        roughness = rma.r;
        metallic = rma.g;
    }

    return true;
}

bool ResolveRayHitWorldNormal(RayQueryTriangleSample triSample, mat4x3 objectToWorld, vec3 rayDirection, RayQuerySurfaceHit hit, out vec3 worldNormal) {
    worldNormal = vec3(0.0, 1.0, 0.0);
    if (!triSample.valid || !hit.hitFound) {
        return false;
    }

    vec3 objectNormal = normalize(
        vec3(triSample.v0.nx, triSample.v0.ny, triSample.v0.nz) * triSample.weights.x +
        vec3(triSample.v1.nx, triSample.v1.ny, triSample.v1.nz) * triSample.weights.y +
        vec3(triSample.v2.nx, triSample.v2.ny, triSample.v2.nz) * triSample.weights.z);

    vec3 objectTangent = normalize(
        vec3(triSample.v0.tx, triSample.v0.ty, triSample.v0.tz) * triSample.weights.x +
        vec3(triSample.v1.tx, triSample.v1.ty, triSample.v1.tz) * triSample.weights.y +
        vec3(triSample.v2.tx, triSample.v2.ty, triSample.v2.tz) * triSample.weights.z);

    worldNormal = normalize(objectToWorld * vec4(objectNormal, 0.0));
    vec3 worldTangent = normalize(objectToWorld * vec4(objectTangent, 0.0));
    worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);
    vec3 worldBitangent = cross(worldNormal, worldTangent);

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[hit.materialIndex];
    if (material.normal >= 0) {
        uint normalTextureIndex = uint(material.normal);
        vec3 normalMap = textureLod(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), hit.uv, 0.0).rgb;
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

        RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pushConstant.data.rayQueryBLASInstanceDataDeviceAddress);
        RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pushConstant.data.rayQueryMeshInstanceDataDeviceAddress);

        // instanceCustomIndex points to the BLAS instance table
        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASInstanceData blasInstanceData = rayQueryBLASInstanceDataBuffer.blasInstanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= blasInstanceData.meshInstanceDataCount) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return 0.0;
        }

        RayQueryMeshInstanceData meshInstanceData = rayQueryMeshInstanceDataBuffer.meshInstanceData[blasInstanceData.meshInstanceDataOffset + geometryIndex];

        // Skip meshes that do not cast point-light shadows
        if ((meshInstanceData.material.shadowBit & SHADOW_FLAG_POINT_LIGHT) == 0u) {
            continue;
        }

        // Skip blended materials (aka eyebrows)
        if (meshInstanceData.material.blendingMode == BLENDING_MODE_BLENDED) {
            continue;
        }

        // Skip alpha-tested materials where the sampled hit pixel is transparent
        if (meshInstanceData.material.blendingMode == BLENDING_MODE_ALPHA_DISCARD ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {

            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            RayQuerySurfaceHit hit = ResolveRayQuerySurfaceHit(meshInstanceData, primitiveIndex, barycentrics, 0.0);
            vec4 hitBaseColor;
            if (SampleRayHitBaseColor(hit, hitBaseColor) &&
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

RayQueryReflectedSurfaceHit RayQueryReflectedSurface(vec3 rayOrigin, vec3 rayDirection) {
    // Finds first non-mirror surface in the reflection ray.
    RayQueryReflectedSurfaceHit result;
    result.surface = EmptyRayQuerySurfaceHit();
    result.normal = vec3(0.0, 1.0, 0.0);

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, 0u, 0xff, rayOrigin, 0.01, rayDirection, 80.0);

    // Skip unusable candidates until one should be committed
    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pushConstant.data.rayQueryBLASInstanceDataDeviceAddress);
        RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pushConstant.data.rayQueryMeshInstanceDataDeviceAddress);

        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASInstanceData blasInstanceData = rayQueryBLASInstanceDataBuffer.blasInstanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= blasInstanceData.meshInstanceDataCount) {
            continue;
        }

        RayQueryMeshInstanceData meshInstanceData = rayQueryMeshInstanceDataBuffer.meshInstanceData[blasInstanceData.meshInstanceDataOffset + geometryIndex];
        if (meshInstanceData.material.blendingMode == BLENDING_MODE_MIRROR) {
            // Mirrors should not reflect themselves forever
            continue;
        }

        uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
        vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

        RayQuerySurfaceHit candidateHit = ResolveRayQuerySurfaceHit(meshInstanceData, primitiveIndex, barycentrics, 0.0);
        vec4 hitBaseColor;
        if (!SampleRayHitBaseColor(candidateHit, hitBaseColor)) {
            continue;
        }

        if (meshInstanceData.material.blendingMode == BLENDING_MODE_ALPHA_DISCARD ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {

            if (hitBaseColor.a < ALPHA_TEST_THRESHOLD) {
                // Transparent pixels are invisible to reflections
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
    }

    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    if (intersectionType == gl_RayQueryCommittedIntersectionNoneEXT) {
        return result;
    }

    RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pushConstant.data.rayQueryBLASInstanceDataDeviceAddress);
    RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pushConstant.data.rayQueryMeshInstanceDataDeviceAddress);

    uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
    RayQueryBLASInstanceData blasInstanceData = rayQueryBLASInstanceDataBuffer.blasInstanceData[instanceIndex];

    uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
    if (geometryIndex >= blasInstanceData.meshInstanceDataCount) {
        return result;
    }

    RayQueryMeshInstanceData meshInstanceData = rayQueryMeshInstanceDataBuffer.meshInstanceData[blasInstanceData.meshInstanceDataOffset + geometryIndex];
    uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    float rayT = rayQueryGetIntersectionTEXT(rayQuery, true);

    RayQueryTriangleSample triangleSample = ResolveRayQueryTriangleSample(meshInstanceData, primitiveIndex, barycentrics);
    if (!triangleSample.valid) {
        return result;
    }

    RayQuerySurfaceHit surface = ResolveRayQuerySurfaceHit(meshInstanceData, triangleSample, rayT);
    if (!surface.hitFound) {
        return result;
    }

    vec3 worldNormal;
    if (!ResolveRayHitWorldNormal(triangleSample, objectToWorld, rayDirection, surface, worldNormal)) {
        return result;
    }

    result.surface = surface;
    result.normal = worldNormal;
    return result;
}

bool TraceRaytracedRadiance(vec3 rayOrigin, vec3 rayDirection, vec3 receiverWorldPos, out vec3 radiance) {
    radiance = vec3(0.0);

    RayQueryReflectedSurfaceHit reflectedHit = RayQueryReflectedSurface(rayOrigin, rayDirection);
    if (!reflectedHit.surface.hitFound) {
        return false;
    }

    vec4 reflectedBaseColor;
    float reflectedRoughness;
    float reflectedMetallic;
    if (!SampleRayHitMaterial(reflectedHit.surface, reflectedBaseColor, reflectedRoughness, reflectedMetallic)) {
        return false;
    }

    vec3 reflectedWorldPos = rayOrigin + rayDirection * reflectedHit.surface.rayT;
    vec3 reflectedLinearBaseColor = pow(reflectedBaseColor.rgb, vec3(2.2));

    LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);
    for (int i = 0; i < LIGHT_COUNT; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightBoundsMin = light.worldBoundsMin.xyz;
        vec3 lightBoundsMax = light.worldBoundsMax.xyz;

        // First broad reject using your tight light bounds
        if (!PointInAABB(reflectedWorldPos, lightBoundsMin, lightBoundsMax)) {
            continue;
        }

        vec3 toLight = lightPosition - reflectedWorldPos;
        float dist = length(toLight);
        vec3 lightDir = toLight / max(dist, 0.0001);
        float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
        float ndotl = max(dot(reflectedHit.normal, lightDir), 0.0);
        if (ndotl <= 0.0 || attenuation <= 0.0) {
            continue;
        }

        // Check line of sight
        float visibility = GetShadowVisibility(reflectedWorldPos + reflectedHit.normal * 0.001, lightPosition);

        // Bail if there is none
        if (visibility <= 0.0) {
            continue;
        }

        radiance += GetDirectLightingForRayHit(
            lightPosition,
            lightColor,
            light.radius,
            light.strength,
            reflectedHit.normal,
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

            vec3 lightBoundsMin = light.worldBoundsMin.xyz;
            vec3 lightBoundsMax = light.worldBoundsMax.xyz;

            // First broad reject using your tight light bounds
            if (!PointInAABB(worldPos, lightBoundsMin, lightBoundsMax)) {
                continue;
            }

            vec3 toLight = lightPosition - worldPos;
            float dist = length(toLight);
            vec3 lightDir = toLight / max(dist, 0.0001);
            float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
            float ndotl = max(dot(normal, lightDir), 0.0);
            if (ndotl <= 0.0 || attenuation <= 0.0) {
                continue;
            }

            // Check line of sight
            float visibility = GetShadowVisibility(worldPos + normal * 0.001, lightPosition);

            // Bail if there is none
            if (visibility <= 0.0) {
                continue;
            }

            // Comptue PBR
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
