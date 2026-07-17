#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/constants.glsl"
#include "../common/flags.glsl"
#include "../common/lighting.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

#include "VK_ddgi_upsample.glsl"

layout(location = 0) out vec4 LightingOut;

layout(location = 0) centroid in vec2 v_texCoord;
layout(location = 1) centroid in vec3 v_normal;
layout(location = 2) centroid in vec3 v_tangent;
layout(location = 3) centroid in vec4 v_worldPos;
layout(location = 4) flat in uint v_globalInstanceIndex;
layout(location = 5) flat in uint v_viewportIndex;

layout(buffer_reference, scalar) readonly buffer RenderItemBuffer {
    RenderItem renderItems[];
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportData[];
};

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(buffer_reference, scalar) readonly buffer LightBuffer {
    Light lights[];
};

struct PackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

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

struct RayQueryMeshInstanceData {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    RayQueryMesh mesh;
    RayQueryMaterial material;
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

const int MAX_GPU_LIGHTS = 16;
const float ALPHA_TEST_THRESHOLD = 0.25;

bool RayQueryMaterialUsesAlphaMask(uint blendingMode) {
    return blendingMode == BLENDING_MODE_ALPHA_DISCARD || blendingMode == BLENDING_MODE_HAIR || blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER;
}

bool RayQueryHitFailsAlphaMask(uint blendingMode, vec4 baseColor) {
    return RayQueryMaterialUsesAlphaMask(blendingMode) && baseColor.a < ALPHA_TEST_THRESHOLD;
}

bool RayQueryMaterialSkipsLineOfSight(uint blendingMode) {
    return blendingMode == BLENDING_MODE_BLENDED;
}

bool SampleRayQueryBaseColor(RayQueryMeshInstanceData meshInstanceData, uint primitiveIndex, vec2 barycentrics, out vec4 baseColor) {
    baseColor = vec4(0.0);

    if (meshInstanceData.material.materialIndex < 0) {
        return false;
    }

    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= meshInstanceData.mesh.indexCount) {
        return false;
    }

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

    vec3 weights = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    vec2 uv = vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[meshInstanceData.material.materialIndex];
    if (material.basecolor < 0) {
        return false;
    }

    uint textureIndex = uint(material.basecolor);
    baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0);
    return true;
}

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

        RayQueryBLASInstanceDataBuffer rayQueryBLASInstanceDataBuffer = RayQueryBLASInstanceDataBuffer(pushConstant.data.rayQueryBLASInstanceDataDeviceAddress);
        RayQueryMeshInstanceDataBuffer rayQueryMeshInstanceDataBuffer = RayQueryMeshInstanceDataBuffer(pushConstant.data.rayQueryMeshInstanceDataDeviceAddress);

        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASInstanceData blasInstanceData = rayQueryBLASInstanceDataBuffer.blasInstanceData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        if (geometryIndex >= blasInstanceData.meshInstanceDataCount) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return 0.0;
        }

        RayQueryMeshInstanceData meshInstanceData = rayQueryMeshInstanceDataBuffer.meshInstanceData[blasInstanceData.meshInstanceDataOffset + geometryIndex];
        if (RayQueryMaterialSkipsLineOfSight(meshInstanceData.material.blendingMode)) {
            continue;
        }

        bool alphaMask = RayQueryMaterialUsesAlphaMask(meshInstanceData.material.blendingMode);
        if (alphaMask) {
            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            vec4 hitBaseColor;
            if (SampleRayQueryBaseColor(meshInstanceData, primitiveIndex, barycentrics, hitBaseColor) && RayQueryHitFailsAlphaMask(meshInstanceData.material.blendingMode, hitBaseColor)) {
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
        return 0.0;
    }

    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

float DirectLightVisibility(vec3 worldPos, vec3 normal, vec3 lightPos) {
    if (pushConstant.data.rayQueryEnabled == 0u) {
        return 1.0;
    }

    return RayQueryLineOfSight(worldPos + normal * 0.001, lightPos);
}

vec3 ComputeDirectLighting(LightBuffer lightBuffer, vec3 worldPos, vec3 normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 directLighting = vec3(0.0);

    for (int i = 0; i < MAX_GPU_LIGHTS; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
        float visibility = 1.0;
        if (light.hiResShadowMapIndex != -1 || light.lowResShadowMapIndex != -1) {
            visibility = DirectLightVisibility(worldPos, normal, lightPosition);
        }

        if (visibility <= 0.0) {
            continue;
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal, worldPos, baseColor, roughness, metallic, viewPos) * visibility;

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            directLight *= ApplyIESProfile(worldPos, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }

        directLighting += directLight;
    }

    return directLighting;
}

vec3 BuildNormal(vec3 vertexNormal, vec3 vertexTangent, vec3 normalMap) {
    vec3 n = normalize(vertexNormal);
    vec3 t = normalize(vertexTangent);
    if (!gl_FrontFacing) {
        n = -n;
    }

    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    normalMap = normalMap * 2.0 - 1.0;
    return normalize(mat3(t, b, n) * normalMap);
}

void main() {
    RenderItemBuffer renderItemBuffer = RenderItemBuffer(pushConstant.data.frame.renderItemsDeviceAddress);
    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.frame.rendererDataDeviceAddress);
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);
    LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);

    RenderItem renderItem = renderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = materialBuffer.materials[renderItem.materialIndex];
    RendererData rendererData = rendererDataBuffer.rendererData;
    ViewportData viewport = viewportDataBuffer.viewportData[v_viewportIndex];

    if (material.basecolor < 0 || material.normal < 0 || material.rma < 0) {
        discard;
    }

    uint baseColorTextureIndex = uint(material.basecolor);
    uint normalTextureIndex = uint(material.normal);
    uint rmaTextureIndex = uint(material.rma);

    vec4 baseColor = texture(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), v_texCoord);
    vec3 normalMap = texture(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), v_texCoord).rgb;
    vec4 rma = texture(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), v_texCoord).rgba;

    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;
    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec3 normal = BuildNormal(v_normal, v_tangent, normalMap);

    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5;
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    vec3 viewPos = viewport.viewPos.xyz;
    vec3 directLighting = ComputeDirectLighting(lightBuffer, v_worldPos.xyz, normal, linearBaseColor, roughness, metallic, viewPos);
    vec3 indirectDiffuse = vec3(0.0);

    if (rendererData.enableIrradianceProbeSampling) {
        ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
        vec2 screenUV = (vec2(gl_FragCoord.xy) + 0.5) / vec2(outputImageSize);
        ivec4 viewportRect = ivec4(viewport.xOffset, viewport.yOffset, viewport.width, viewport.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, normal, distance(v_worldPos.xyz, viewPos), outputImageSize, viewportRect);
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        indirectDiffuse = probeIrradiance * diffuseAlbedo;
    }

    vec3 finalLitColor = (directLighting + indirectDiffuse) * ao;

    LightingOut.rgb = finalLitColor;
    LightingOut.a = baseColor.a;
}
