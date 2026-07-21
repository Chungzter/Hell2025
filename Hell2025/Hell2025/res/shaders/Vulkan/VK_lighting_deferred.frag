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
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

#include "VK_point_shadows.glsl"

#include "VK_ddgi_upsample.glsl"
#include "VK_indirect_specular_amd_apply.glsl"

layout(early_fragment_tests) in;
layout(location = 0) out vec4 out_color;

#define LIGHT_COUNT 9             // TODO "tile based deferred" me the fuck outta here
#define ALPHA_TEST_THRESHOLD 0.25 // TODO: Make me instance specific, which means adding it to the RenderItem struct

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

struct RayHit {
    bool found;
    vec3 hitPos;
    vec3 hitNormal;
    vec3 rayDir;
    float rayT;
    int materialIndex;
    vec2 uv;
};

struct Surface {
    vec3 worldPos;
    vec3 normal;
    vec3 linearBaseColor;
    float roughness;
    float metallic;
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
} pc;

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

vec3 EvaluatePointLight(vec3 lightPos, vec3 lightColor, float lightRadius, float lightStrength, vec3 surfacePos, vec3 normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    // Build the light and view directions
    vec3 toLight = lightPos - surfacePos;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 0.0001);
    vec3 viewDir = normalize(viewPos - surfacePos);
    float attenuation = smoothstep(lightRadius, 0.0, dist) * lightStrength;
    float ndotl = max(dot(normal, lightDir), 0.0);

    // Reject surfaces this light cannot reach
    if (ndotl <= 0.0 || attenuation <= 0.0) {
        return vec3(0.0);
    }

    // Evaluate the direct BRDF
    vec3 brdf = microfacetBRDF(lightDir, viewDir, normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndotl * attenuation * clamp(lightColor, 0.0, 1.0);
}

bool PassesAlphaTest(RayQueryMeshInstanceData meshInstanceData, uint primitiveIndex, vec2 barycentrics) {
    // Reject broken material data
    if (meshInstanceData.material.materialIndex < 0) {
        return false;
    }

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= meshInstanceData.mesh.indexCount) {
        return false;
    }

    // Fetch the candidate triangle
    IndexBuffer indexBuffer = IndexBuffer(meshInstanceData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 0u] + meshInstanceData.mesh.baseVertex;
    uint i1 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 1u] + meshInstanceData.mesh.baseVertex;
    uint i2 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 2u] + meshInstanceData.mesh.baseVertex;
    uint vertexEnd = meshInstanceData.mesh.baseVertex + meshInstanceData.mesh.vertexCount;
    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return false;
    }

    VertexBuffer vertexBuffer = VertexBuffer(meshInstanceData.vertexBufferDeviceAddress);
    PackedVertex v0 = vertexBuffer.vertices[i0];
    PackedVertex v1 = vertexBuffer.vertices[i1];
    PackedVertex v2 = vertexBuffer.vertices[i2];

    // Interpolate the candidate UV
    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec2 uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    Material material = materialBuffer.materials[meshInstanceData.material.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    // Sample only the alpha channel
    uint textureIndex = uint(material.basecolor);
    float alpha = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0).a;
    return alpha >= ALPHA_TEST_THRESHOLD;
}

Surface SurfaceFromRayHit(RayHit rayhit) {
    // Copy the hit geometry
    Surface surface;
    surface.worldPos = rayhit.hitPos;
    surface.normal = rayhit.hitNormal;
    surface.linearBaseColor = vec3(0.0);
    surface.roughness = 1.0;
    surface.metallic = 0.0;

    // Sample the linear base color
    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    Material material = materialBuffer.materials[rayhit.materialIndex];

    uint textureIndex = uint(material.basecolor);
    vec3 baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), rayhit.uv, 0.0).rgb;
    surface.linearBaseColor = pow(baseColor, vec3(2.2));

    // Sample roughness and metallic when available
    if (material.rma >= 0) {
        uint rmaTextureIndex = uint(material.rma);
        vec4 rma = textureLod(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), rayhit.uv, 0.0);
        surface.roughness = rma.r;
        surface.metallic = rma.g;
    }

    return surface;
}

bool AnyHit(vec3 rayOrigin, vec3 rayDir, float maxDistance) {
    // Return true when anything blocks this shadow ray
    rayQueryEXT rayQuery;
    // Shadow flags, blending modes, and alpha masks must be checked for every triangle.
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, gl_RayFlagsNoOpaqueEXT, 0xff, rayOrigin, 0.001, rayDir, maxDistance);

    // Walk candidates until a blocker is accepted or the BVH ends
    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pc.data.rayQueryBLASInstanceDataDeviceAddress);
        RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pc.data.rayQueryMeshInstanceDataDeviceAddress);

        // instanceCustomIndex points to the BLAS instance table
        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASInstanceData blasInstanceData = rayQueryBLASInstanceDataBuffer.blasInstanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= blasInstanceData.meshInstanceDataCount) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return true;
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

            if (!PassesAlphaTest(meshInstanceData, primitiveIndex, barycentrics)) {
                continue;
            }
        }

        // This hit blocks the light
        rayQueryConfirmIntersectionEXT(rayQuery);
        return true;
    }

    // A committed hit means the ray is blocked
    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType != gl_RayQueryCommittedIntersectionNoneEXT;
}

RayHit ClosestHit(vec3 rayOrigin, vec3 rayDir, float maxDistance) {
    // Start with no hit
    RayHit result;
    result.found = false;
    result.hitPos = vec3(0.0);
    result.hitNormal = vec3(0.0, 1.0, 0.0);
    result.rayDir = rayDir;
    result.rayT = 0.0;
    result.materialIndex = -1;
    result.uv = vec2(0.0);

    // Find the closest surface this reflection ray can see
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, 0u, 0xff, rayOrigin, 0.01, rayDir, maxDistance);

    // Reject candidates that reflections should pass through
    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pc.data.rayQueryBLASInstanceDataDeviceAddress);
        RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pc.data.rayQueryMeshInstanceDataDeviceAddress);

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

        if (meshInstanceData.material.materialIndex < 0) {
            continue;
        }

        MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
        Material material = materialBuffer.materials[meshInstanceData.material.materialIndex];
        if (material.basecolor < 0) {
            continue;
        }

        uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
        vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

        if (meshInstanceData.material.blendingMode == BLENDING_MODE_ALPHA_DISCARD ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR ||
            meshInstanceData.material.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {

            if (!PassesAlphaTest(meshInstanceData, primitiveIndex, barycentrics)) {
                // Transparent pixels are invisible to reflections
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
    }

    // Return no hit if nothing survived the candidate filters
    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    if (intersectionType == gl_RayQueryCommittedIntersectionNoneEXT) {
        return result;
    }

    // Resolve the committed mesh and triangle
    RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pc.data.rayQueryBLASInstanceDataDeviceAddress);
    RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pc.data.rayQueryMeshInstanceDataDeviceAddress);

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

    if (meshInstanceData.material.materialIndex < 0) {
        return result;
    }

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= meshInstanceData.mesh.indexCount) {
        return result;
    }

    // Fetch the three hit vertices
    IndexBuffer indexBuffer = IndexBuffer(meshInstanceData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 0u] + meshInstanceData.mesh.baseVertex;
    uint i1 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 1u] + meshInstanceData.mesh.baseVertex;
    uint i2 = indexBuffer.indices[meshInstanceData.mesh.baseIndex + localIndexOffset + 2u] + meshInstanceData.mesh.baseVertex;
    uint vertexEnd = meshInstanceData.mesh.baseVertex + meshInstanceData.mesh.vertexCount;
    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return result;
    }

    VertexBuffer vertexBuffer = VertexBuffer(meshInstanceData.vertexBufferDeviceAddress);
    PackedVertex v0 = vertexBuffer.vertices[i0];
    PackedVertex v1 = vertexBuffer.vertices[i1];
    PackedVertex v2 = vertexBuffer.vertices[i2];
    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

    // Fill the final hit data
    result.found = true;
    result.hitPos = rayOrigin + rayDir * rayT;
    result.rayT = rayT;
    result.materialIndex = meshInstanceData.material.materialIndex;
    result.uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

    // Interpolate and transform the vertex normal and tangent
    vec3 objectNormal = normalize(vec3(v0.nx, v0.ny, v0.nz) * weights.x + vec3(v1.nx, v1.ny, v1.nz) * weights.y + vec3(v2.nx, v2.ny, v2.nz) * weights.z);
    vec3 objectTangent = normalize(vec3(v0.tx, v0.ty, v0.tz) * weights.x + vec3(v1.tx, v1.ty, v1.tz) * weights.y + vec3(v2.tx, v2.ty, v2.tz) * weights.z);
    result.hitNormal = normalize(objectToWorld * vec4(objectNormal, 0.0));
    vec3 worldTangent = normalize(objectToWorld * vec4(objectTangent, 0.0));
    worldTangent = normalize(worldTangent - dot(worldTangent, result.hitNormal) * result.hitNormal);
    vec3 worldBitangent = cross(result.hitNormal, worldTangent);

    // Apply the material normal map
    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    Material material = materialBuffer.materials[result.materialIndex];
    if (material.normal >= 0) {
        uint normalTextureIndex = uint(material.normal);
        vec3 normalMap = textureLod(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), result.uv, 0.0).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        result.hitNormal = normalize(mat3(worldTangent, worldBitangent, result.hitNormal) * normalMap);
    }

    // Make the normal face against the ray
    if (dot(result.hitNormal, rayDir) > 0.0) {
        result.hitNormal = -result.hitNormal;
    }

    return result;
}

float GetShadowVisibility(vec3 rayOrigin, vec3 target) {

    // Build a ray that stops before the light
    vec3 rayVector = target - rayOrigin;
    float rayLength = length(rayVector);

    const float rayTMin = 0.001;
    const float targetBias = 0.01;

    float rayTMax = rayLength - targetBias;

    // Treat very short rays as visible
    if (rayTMax <= rayTMin) {
        return 1.0;
    }

    vec3 rayDir = rayVector / rayLength;

    const int shadowSampleCount = 1;
    const float shadowLightSize = 0.0;

    // Jitter and average the shadow samples
    float visibility = 0.0;
    for (int i = 0; i < shadowSampleCount; i++) {
        vec3 jitteredRayDir = GetJitterRay(rayDir, shadowLightSize, float(i));
        visibility += AnyHit(rayOrigin, jitteredRayDir, rayTMax) ? 0.0 : 1.0;
    }

    return visibility / float(shadowSampleCount);
}

void main() {
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    TileLightsBuffer tileLights = pc.data.frame.tileLightBuffer;

    RendererData rendererData = rendererDataBuffer.rendererData;

    // Get viewport data
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    vec2 screenUV = (vec2(px) + 0.5) / vec2(outputImageSize);
    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, outputImageSize, rendererDataBuffer.rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportData[viewportIndex];
    vec3 viewPos = viewportData.viewPos.xyz;

    // Fetch GBuffer
    vec4 baseColorMetallic = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    vec4 normalXYRoughnessMisc = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    // Reconstruct position from depth
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);
    vec3 worldPos = WorldPosFromDepth_VK(viewportUV, depth, viewportData.inverseProjectionViewReverseZ);
    float fragDistance = distance(worldPos, viewPos);

    // Reconstruct materials
    vec3 baseColor = baseColorMetallic.rgb;
    vec3 normal = DecodeOct(normalXYRoughnessMisc.rg);
    float metallic = baseColorMetallic.a;
    float roughness = normalXYRoughnessMisc.b;

    vec3 linearBaseColor = pow(baseColor, vec3(2.2));

    Surface surface;
    surface.worldPos = worldPos;
    surface.normal = normal;
    surface.linearBaseColor = linearBaseColor;
    surface.roughness = roughness;
    surface.metallic = metallic;

    // Decode flags
    uint miscFlags = DecodeMiscFlags(normalXYRoughnessMisc.a);
    bool isMirrorSurface = (miscFlags & MISC_FLAG_MIRROR_SURFACE) != 0u;

    // Direct light
    vec3 directLighting = vec3(0.0);

    
    LightBuffer lightBuffer = pc.data.frame.lightBuffer;

    // Tile data
    uvec2 tileCoord = uvec2(px) / uint(TILE_SIZE);
    uint tileIndex = tileCoord.y * rendererData.tileCountX + tileCoord.x;
    uint tileLightCount = tileLights.tileLights[tileIndex].lightCount;

    // Direct lighting
    for (int i = 0; i < tileLightCount; i++) {

        int lightIndex = int(tileLights.tileLights[tileIndex].lightIndices[i]);
        Light light = lightBuffer.lights[lightIndex];

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightBoundsMin = light.worldBoundsMin.xyz;
        vec3 lightBoundsMax = light.worldBoundsMax.xyz;

        float candelas = 1.0;

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            candelas = ApplyIESProfile(worldPos, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }

        if (candelas == 0) {
            continue;
        }

        float visibility = rendererData.directPointShadowMode == POINT_SHADOW_MODE_RAY_QUERY
            ? GetShadowVisibility(worldPos + normal * 0.001, lightPosition)
            : GetPointShadowMapVisibilitySkin(light, worldPos, normal, viewPos);
        visibility *= candelas;

        if (visibility <= 0.0) {
            continue;
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * visibility;
        directLighting += directLight;
    }

    ivec4 viewportRect = ivec4(viewportData.xOffset, viewportData.yOffset, viewportData.width, viewportData.height);

    // Indirect specular
    vec3 indirectSpecular = vec3(0, 0, 0);

    if (rendererData.enableIndirectSpecular) {
        indirectSpecular = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_TEMPORAL], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).rgb;
        float amdAlphaRoughness = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_MATERIAL_ROUGHNESS], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
        float amdPerceptualRoughness = sqrt(clamp(amdAlphaRoughness, 0.0, 1.0));
        vec3 viewDirToCamera = normalize(viewPos - worldPos);

        vec3 primaryBaseColor = isMirrorSurface ? vec3(1.0) : linearBaseColor;
        float primaryMetallic = isMirrorSurface ? 1.0 : metallic;
        vec3 primaryResponse = GetAMDIndirectSpecularPrimaryResponse(primaryBaseColor, primaryMetallic, amdPerceptualRoughness, normal, viewDirToCamera, pc.data.brdfLutTextureIndex);
        float indirectSpecularFactor = isMirrorSurface ? 1.0 : rendererData.indirectSpecularFactor; // Mirrors always get a boost factor of 1.0, any other reflective surface uses the RendererData value

        indirectSpecular = ApplyAMDIndirectSpecularBRDF(indirectSpecular, primaryResponse) * indirectSpecularFactor;
    }

    // Indirect diffuse
    vec3 indirectDiffuse = vec3(0.0);

    if (!isMirrorSurface && rendererData.enableIrradianceProbeSampling) {
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, normal, fragDistance, outputImageSize, viewportRect);
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        indirectDiffuse = probeIrradiance * diffuseAlbedo;
    }

    // Final composite
    vec3 finalLighting = directLighting + indirectDiffuse + indirectSpecular;

    out_color = vec4(finalLighting, 1.0);


   // out_color = vec4(indirectSpecular, 1.0);


    // Normals test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayHit reflectedHit = ClosestHit(testRayOrigin, testRayDir, 80.0);

            if (reflectedHit.found) {
                vec2 normalOct = EncodeOct(reflectedHit.hitNormal); // write/read this
                vec3 reconstructedNormal = DecodeOct(normalOct);
                debugColor = clamp(reconstructedNormal, 0, 1);
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

    // World pos test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayHit reflectedHit = ClosestHit(testRayOrigin, testRayDir, 80.0);

            if (reflectedHit.found) {
                vec2 rayDirOct = EncodeOct(testRayDir) * 2.0 - 1.0; // write/read this
                float rayT = reflectedHit.rayT; // write/read this
                vec3 reconstructedRayDir = DecodeOct(rayDirOct * 0.5 + 0.5);
                vec3 reconstructedWorldPos = testRayOrigin + reconstructedRayDir * rayT;
                debugColor = reconstructedWorldPos;
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

    // Material test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayHit reflectedHit = ClosestHit(testRayOrigin, testRayDir, 80.0);

            if (reflectedHit.found) {
                vec2 uv = reflectedHit.uv; // write/read this
                uint materialIndex = uint(reflectedHit.materialIndex); // write/read this

                MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
                Material material = materialBuffer.materials[int(materialIndex)];
                if (material.basecolor >= 0) {
                    uint textureIndex = uint(material.basecolor);
                    vec3 baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0).rgb;
                    debugColor = pow(baseColor, vec3(2.2));
                }
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

}
