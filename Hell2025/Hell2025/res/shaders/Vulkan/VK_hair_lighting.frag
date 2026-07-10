#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/hair.glsl"
#include "../common/Vulkan/binding_indices.glsl"
#include "../common/constants.glsl"
#include "../common/flags.glsl"
#include "../common/lighting.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"
#include "../common/Vulkan/push_constants.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

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

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportData[];
};

layout(buffer_reference, scalar) readonly buffer LightBuffer {
    Light lights[];
};

// Must match the ray query vertex buffer layout.
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
    PushConstantsHair data;
} pushConstant;

const int MAX_GPU_LIGHTS = 16;
const float u_spec1Intensity   = 0.25;
const float u_spec2Intensity   = 0.1;
const float u_scatterPower     = 12.0;
const float u_scatterIntensity = 0.1;
const float u_rootColorFloor   = 0.2;
const float u_rootAOFloor      = 0.7;
const float u_tipColorFloor    = 0.45;
const float u_tipAOFloor       = 0.7;

float HairSpecular(vec3 t, vec3 h, float roughness) {
    float alpha = max(roughness * roughness, 0.001);
    float n = 0.36 / alpha;
    float dotTH = dot(t, h);
    float sinTH = sqrt(max(0.0, 1.0 - dotTH * dotTH));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, n) * (n + 2.0) / (2.0 * 3.14159);
}

vec3 EvaluateHairLight(vec3 hairBaseColor, vec3 finalTangent, vec3 V, vec3 L, vec3 t1, vec3 t2, float alpha1, float alpha2, vec3 lightColor, float visibility) {
    vec3 H = normalize(L + V);

    float dotTL = dot(finalTangent, L);
    float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));
    vec3 diffuse = hairBaseColor * sinTL;

    float D1 = HairSpecular(t1, H, alpha1);
    float D2 = HairSpecular(t2, H, alpha2);

    float dotVH = clamp(dot(V, H), 0.0, 1.0);
    float fresnel = pow(1.0 - dotVH, 5.0);

    vec3 F1 = vec3(0.04) + vec3(0.96) * fresnel;
    vec3 F2 = hairBaseColor + (vec3(1.0) - hairBaseColor) * fresnel;

    vec3 spec1 = D1 * F1 * u_spec1Intensity;
    vec3 spec2 = D2 * F2 * hairBaseColor * u_spec2Intensity;

    float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
    vec3 scattering = hairBaseColor * scatterProp * u_scatterIntensity;

    return (diffuse + spec1 + spec2 + scattering) * clamp(lightColor, 0.0, 1.0) * visibility;
}

void ComputeCCNormalAndTangents(vec3 vertexNormal, vec3 vertexTangent, vec3 flowMap, float hairID, float flipTangentGreen, out vec3 finalNormal, out vec3 finalTangent) {
    vec3 meshTangent = normalize(vertexTangent);
    vec3 meshNormalUnflipped = normalize(vertexNormal);
    vec3 meshNormal = gl_FrontFacing ? meshNormalUnflipped : -meshNormalUnflipped;
    vec3 meshBitangent = normalize(cross(meshNormalUnflipped, meshTangent));

    flowMap = flowMap * 2.0 - 1.0;

    vec3 tangentSpaceShift;
    tangentSpaceShift.x = flowMap.x;
    tangentSpaceShift.y = flowMap.y * flipTangentGreen;
    tangentSpaceShift.z = 0.0;

    vec3 blackOffset = vec3(-0.206, -0.687, -0.338);
    vec3 whiteOffset = vec3(-0.148, 0.0, 0.370);
    vec3 idOffset = mix(blackOffset, whiteOffset, hairID);

    tangentSpaceShift = normalize(tangentSpaceShift + idOffset);

    finalTangent = vec3(
        tangentSpaceShift.x * meshTangent +
        tangentSpaceShift.y * meshBitangent +
        tangentSpaceShift.z * meshNormal
    );

    finalNormal = meshNormal;
}

bool RayQueryGeometryNeedsAlphaTest(uint blendingMode) {
    return blendingMode == BLENDING_MODE_ALPHA_DISCARD || blendingMode == BLENDING_MODE_HAIR || blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER;
}

bool SampleRayQueryBaseColor(RayQueryGeometryData geometryData, uint primitiveIndex, vec2 barycentrics, out vec4 baseColor) {
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

    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    Material material = materialBuffer.materials[geometryData.materialIndex];
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
        if (geometryData.blendingMode == BLENDING_MODE_HAIR || geometryData.blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER) {
            continue;
        }

        bool alphaTest = RayQueryGeometryNeedsAlphaTest(geometryData.blendingMode) && geometryData.materialIndex >= 0;

        if (alphaTest) {
            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            vec4 hitBaseColor;
            if (SampleRayQueryBaseColor(geometryData, primitiveIndex, barycentrics, hitBaseColor) && hitBaseColor.a < 0.25) {
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
        return 0.0;
    }

    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

float HairLightVisibility(vec3 worldPos, vec3 normal, vec3 lightPos) {
    if (pushConstant.data.rayQueryEnabled == 0u) {
        return 1.0;
    }

    return RayQueryLineOfSight(worldPos + normal * 0.001, lightPos);
}

void AddPointLights(inout vec3 directLighting, LightBuffer lightBuffer, vec3 hairBaseColor, vec3 finalTangent, vec3 finalNormal, vec3 V, vec3 t1, vec3 t2, float alpha1, float alpha2) {
    for (int i = 0; i < MAX_GPU_LIGHTS; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);
        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        float lightDistance = sqrt(distanceSquared);
        float attenuation = smoothstep(light.radius, 0.0, lightDistance) * light.strength;
        if (attenuation <= 0.0) {
            continue;
        }

        vec3 L = lightVector * inversesqrt(distanceSquared);
        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1 || light.lowResShadowMapIndex != -1) {
            shadow = HairLightVisibility(v_worldPos.xyz, finalNormal, lightPos);
        }

        vec3 lightContribution = EvaluateHairLight(hairBaseColor, finalTangent, V, L, t1, t2, alpha1, alpha2, lightCol, shadow);

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }

        directLighting += lightContribution * attenuation;
    }
}

void AddFlashlights(inout vec3 directLighting, ViewportDataBuffer viewportDataBuffer, vec3 viewPos, vec3 hairBaseColor, vec3 finalTangent, vec3 finalNormal, vec3 V, vec3 t1, vec3 t2, float alpha1, float alpha2) {
    float fragDistance = distance(v_worldPos.xyz, viewPos);

    for (int i = 0; i < 2; i++) {
        ViewportData flashlightViewportData = viewportDataBuffer.viewportData[i];
        float flashlightModifer = flashlightViewportData.flashlightModifer;
        if (flashlightModifer <= 0.05) {
            continue;
        }

        vec3 spotLightPos = flashlightViewportData.flashlightPosition.xyz;
        vec3 spotLightDir = normalize(flashlightViewportData.flashlightDir.xyz);
        vec3 spotLightColor = GetFlashLightColor();
        float spotLightRadius = 25.0;
        float spotLightStrength = 4.5;

        if (i != int(v_viewportIndex)) {
            spotLightPos += spotLightDir * 0.2;
            spotLightColor *= 0.825;
        }

        float innerAngle = cos(radians(5.0 * flashlightModifer));
        float outerAngle = cos(radians(20.5));
        bool flashlightIsInShop = bool(flashlightViewportData.isInShop);
        if (flashlightIsInShop) {
            spotLightRadius = 8.0;
            outerAngle = cos(radians(50.0));
        }

        vec3 lightVector = spotLightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        float lightDistance = sqrt(distanceSquared);
        float attenuation = smoothstep(spotLightRadius, 0.0, lightDistance) * spotLightStrength;
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float coneFalloff = smoothstep(outerAngle, innerAngle, dot(L, -spotLightDir));
        float distanceFactor = clamp(1.0 - lightDistance / spotLightRadius, 0.0, 1.0);
        float spotAttenuation = attenuation * coneFalloff * distanceFactor * distanceFactor;
        if (spotAttenuation <= 0.0) {
            continue;
        }

        mat4 lightProjectionView = flashlightViewportData.flashlightProjectionView;
        float visibility = 1.0;
        if (!(i == int(v_viewportIndex) && flashlightIsInShop)) {
            visibility = HairLightVisibility(v_worldPos.xyz, finalNormal, spotLightPos);
        }

        if (visibility <= 0.0) {
            continue;
        }

        vec3 lightContribution = EvaluateHairLight(hairBaseColor, finalTangent, V, L, t1, t2, alpha1, alpha2, spotLightColor, visibility);

        vec3 cookie = vec3(1.0);
        if (pushConstant.data.flashlightCookieTextureIndex >= 0) {
            uint cookieTextureIndex = uint(pushConstant.data.flashlightCookieTextureIndex);
            cookie = ApplyCookie(lightProjectionView, v_worldPos.xyz, spotLightPos, spotLightColor, spotLightRadius, textures[nonuniformEXT(cookieTextureIndex)], textureSamplers[nonuniformEXT(cookieTextureIndex)]);
        }

        float cookieStartDistance = 1.0;
        float cookieEndDistance = 10.0;
        float cookieDistanceExponent = 2.0;
        float cookieMinValue = 0.5;
        float cookieMaxValue = 5.0;
        float cookieDistScale;
        if (fragDistance <= cookieStartDistance) {
            cookieDistScale = cookieMinValue;
        }
        else if (fragDistance >= cookieEndDistance) {
            cookieDistScale = cookieMaxValue;
        }
        else {
            float t = (fragDistance - cookieStartDistance) / (cookieEndDistance - cookieStartDistance);
            cookieDistScale = mix(cookieMinValue, cookieMaxValue, pow(t, cookieDistanceExponent));
        }

        lightContribution *= cookieDistScale;
        if (!flashlightIsInShop) {
            lightContribution *= cookie;
        }

        directLighting += lightContribution * spotAttenuation * flashlightModifer;
    }
}

void main() {
    RenderItemBuffer renderItemBuffer = RenderItemBuffer(pushConstant.data.frame.renderItemsDeviceAddress);
    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.frame.materialsDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.frame.rendererDataDeviceAddress);
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);
    LightBuffer lightBuffer = LightBuffer(pushConstant.data.frame.lightsDeviceAddress);

    RenderItem renderItem = renderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = materialBuffer.materials[renderItem.materialIndex];
    ViewportData viewport = viewportDataBuffer.viewportData[v_viewportIndex];

    if (material.basecolor < 0 || material.rma < 0 || material.hairMaps < 0) {
        discard;
    }

    uint baseColorTextureIndex = uint(material.basecolor);
    uint rmaTextureIndex = uint(material.rma);
    uint hairTextureIndex = uint(material.hairMaps);

    vec2 baseTextureSizePixels = vec2(textureSize(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), 0));

    vec4 baseColor = texture(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), v_texCoord);
    vec4 rma = texture(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), v_texCoord).rgba;
    vec4 hairTexture = texture(sampler2D(textures[nonuniformEXT(hairTextureIndex)], textureSamplers[nonuniformEXT(hairTextureIndex)]), v_texCoord);

    vec3 flowMap = vec3(hairTexture.rg, 0.0);
    float hairID = hairTexture.b;
    float rootFactor = hairTexture.a;

    float hairMipLevelRaw = ComputeHairMipLevel(v_texCoord, baseTextureSizePixels);
    float roughness = rma.r;
    float metallic = 1.0;
    float ao = rma.b;
    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec3 viewPos = viewport.viewPos.xyz;
    vec3 V = normalize(viewPos - v_worldPos.xyz);

    vec3 finalNormal;
    vec3 finalTangent;
    ComputeCCNormalAndTangents(v_normal, v_tangent, flowMap, hairID, 1.0, finalNormal, finalTangent);

    vec3 hairBaseColor = linearBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    hairBaseColor *= 0.8;

    const float u_specularAARoughnessPerMip = 0.5;
    const float u_specularMipFadeStrength = 0.2;
    const float u_specularMipStart = 0.9;
    float renderResolutionScale = 1.0;
    float mipLevelRaw = max(0.0, hairMipLevelRaw + log2(renderResolutionScale));
    float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);
    float roughnessAA = clamp(roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);
    float specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

    const float kHairRoughnessMapStrength = 0.45;
    const float kRoughnessGamma = 1.0;
    const float kRoughnessWeight = 1.0;
    float ue4Roughness = pow(abs(rma.r), kRoughnessGamma) * kRoughnessWeight * kHairRoughnessMapStrength;

    const float u_specularAlpha1Min = 0.055;
    const float u_specularAlpha2Min = 0.070;
    float alpha1 = clamp(ue4Roughness * ue4Roughness, u_specularAlpha1Min, 1.0);
    float alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, u_specularAlpha2Min, 1.0);

    vec3 t1 = normalize(finalTangent + finalNormal * 0.035);
    vec3 t2 = normalize(finalTangent - finalNormal * 0.052);

    vec3 directLighting = vec3(0.0);
    AddPointLights(directLighting, lightBuffer, hairBaseColor, finalTangent, finalNormal, V, t1, t2, alpha1, alpha2);
    AddFlashlights(directLighting, viewportDataBuffer, viewPos, hairBaseColor, finalTangent, finalNormal, V, t1, t2, alpha1, alpha2);

    // OpenGL samples indirect diffuse here so setting this to false
    bool u_sampleProbes = false;
    vec2 resolution = vec2(rendererDataBuffer.rendererData.gBufferWidth, rendererDataBuffer.rendererData.gBufferHeight);
    vec2 screenUV = (vec2(gl_FragCoord.xy) + 0.5) / resolution;
    vec3 probeIrradiance = vec3(0.0);

    vec3 indirectDiffuse = vec3(0.0);
    vec3 diffuseAlbedo = hairBaseColor.rgb * (1.0 - metallic);
    float indirectDiffuseScale = 1.0;

    if (u_sampleProbes) {
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    vec3 color = (directLighting + indirectDiffuse) * ao;
    color += vec3(0.00001);

    LightingOut = vec4(color, 1.0);
}
