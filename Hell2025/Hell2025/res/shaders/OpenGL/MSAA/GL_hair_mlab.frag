#version 460 core

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#extension GL_ARB_fragment_shader_interlock : enable
#extension GL_ARB_bindless_texture : enable

layout(early_fragment_tests) in;
layout(pixel_interlock_unordered) in;

layout(std430, binding = 6) coherent buffer HairMLAB {
    uvec4 nodes_v4[];
};

#if ENABLE_BINDLESS == 1
    readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
#else
    layout (binding = 0) uniform sampler2D baseColorTexture;
    layout (binding = 1) uniform sampler2D normalTexture;
    layout (binding = 2) uniform sampler2D rmaTexture;
    layout (binding = 3) uniform sampler2D emissiveTexture;
    layout (binding = 4) uniform sampler2D woundBaseColorTexture;
    layout (binding = 5) uniform sampler2D woundNormalTexture;
    layout (binding = 6) uniform sampler2D woundRmaTexture;
#endif

layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;
layout (binding = 9) uniform samplerCubeArray shadowMapArray;
layout (binding = 11) uniform sampler2D hairFlowMap;
layout (binding = 12) uniform sampler2D hairIdMap;
layout (binding = 13) uniform sampler2D hairRootMap;

#include "../../common/hair.glsl"
#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer   { TileLights tileLights[]; };

centroid in vec2 TexCoord;
centroid in vec3 Normal;
centroid in vec3 Tangent;
centroid in vec4 WorldPos;
centroid in vec3 ViewPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform float u_renderResolutionScale;
uniform int u_renderTargetWidth;
uniform uint u_mlabFrameIndex;

const float u_spec1Intensity   = 0.25;
const float u_spec2Intensity   = 0.9;
const float u_scatterPower     = 12.0;
const float u_scatterIntensity = 0.1;
const float u_specularJitter   = 0.5;
const float u_rootColorFloor   = 0.2;
const float u_rootAOFloor      = 0.7;
const float u_tipColorFloor    = 0.45;
const float u_tipAOFloor       = 0.7;

const float u_specularAARoughnessPerMip = 0.5;
const float u_specularMipFadeStrength = 0.3;
const float u_specularAlpha1Min = 0.055;
const float u_specularAlpha2Min = 0.070;
const float u_specularMipStart = 0.9;

struct HairTextureSamples {
    vec4 baseColor;
    vec4 rma;
    vec4 additionalColor0;
    vec4 additionalColor1;
    vec4 additionalColor2;
    vec2 baseTextureSizePixels;
};

struct HairSurface {
    vec3 baseColor;
    vec3 hairBaseColor;
    vec3 normal;
    vec3 tangent;
    vec3 jitteredTangent;
    float alpha;
    float roughness;
    float metallic;
    float hairAO;
    float alpha1;
    float alpha2;
    float specularMipFade;
};

float HairSpecular(vec3 t, vec3 h, float roughness) {
    float alpha = max(roughness * roughness, 0.001);
    float n = 0.36 / alpha;
    float dotTH = dot(t, h);
    float sinTH = sqrt(max(0.0, 1.0 - dotTH * dotTH));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, n) * (n + 2.0) / (2.0 * 3.14159);
}

vec3 SafeNormalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);

    if (len2 < 0.0001) {
        return normalize(fallback);
    }

    return v * inversesqrt(len2);
}

vec3 BuildHairTangentFromFlow(vec3 tMesh, vec3 bMesh, vec2 flow) {
    float flowLen2 = dot(flow, flow);

    if (flowLen2 < 0.0001) {
        return normalize(tMesh);
    }

    vec2 flowDir = flow * inversesqrt(flowLen2);
    return normalize(tMesh * flowDir.x + bMesh * flowDir.y);
}

HairTextureSamples SampleHairTextures(RenderItem renderItem) {
    HairTextureSamples samples;

#if ENABLE_BINDLESS == 1
    sampler2D baseSampler = sampler2D(textureSamplers[renderItem.baseColorTextureIndex]);

    samples.baseColor = texture(baseSampler, TexCoord);
    samples.rma = texture(sampler2D(textureSamplers[renderItem.rmaTextureIndex]), TexCoord).rgba;
    samples.additionalColor0 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex0]), TexCoord);
    samples.additionalColor1 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex1]), TexCoord);
    samples.additionalColor2 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex2]), TexCoord);
    samples.baseTextureSizePixels = vec2(textureSize(baseSampler, 0));
#else
    samples.baseColor = texture(baseColorTexture, TexCoord);
    samples.rma = texture(rmaTexture, TexCoord).rgba;
    samples.additionalColor0 = texture(hairFlowMap, TexCoord);
    samples.additionalColor1 = texture(hairIdMap, TexCoord);
    samples.additionalColor2 = texture(hairRootMap, TexCoord);
    samples.baseTextureSizePixels = vec2(textureSize(baseColorTexture, 0));
#endif

    return samples;
}

HairSurface BuildHairSurface(HairTextureSamples samples, float coverageAlpha, float hairMipLevelRaw) {
    HairSurface surface;

    vec3 flowSample = samples.additionalColor0.rgb;
    vec2 flow = flowSample.rg * 2.0 - 1.0;
    float strandID = samples.additionalColor1.r;
    float rootFactor = samples.additionalColor2.r;

    surface.alpha = coverageAlpha;
    surface.roughness = samples.rma.r;
    surface.metallic = samples.rma.g;

    float ao = samples.rma.b;
    vec3 linearBaseColor = pow(samples.baseColor.rgb, vec3(2.2));

    vec3 n = normalize(Normal);

    if (!gl_FrontFacing) {
        n = -n;
    }

    vec3 tMesh = normalize(Tangent);
    tMesh = normalize(tMesh - dot(tMesh, n) * n);
    vec3 bMesh = cross(n, tMesh);

    vec3 t = BuildHairTangentFromFlow(tMesh, bMesh, flow);
    vec3 jitteredT = SafeNormalize(t + n * (strandID - 0.5) * u_specularJitter, t);

    surface.baseColor = linearBaseColor;
    surface.hairBaseColor = linearBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    surface.hairAO = ao * mix(u_rootAOFloor, u_tipAOFloor, rootFactor);
    surface.normal = n;
    surface.tangent = t;
    surface.jitteredTangent = jitteredT;

    float mipLevelRaw = max(0.0, hairMipLevelRaw + log2(u_renderResolutionScale));
    float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);
    float roughnessAA = clamp(surface.roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);

    surface.specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

    float ue4Roughness = mix(0.15, 0.25, roughnessAA);

    surface.alpha1 = clamp(ue4Roughness * ue4Roughness, u_specularAlpha1Min, 1.0);
    surface.alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, u_specularAlpha2Min, 1.0);

    return surface;
}

vec3 ComputeHairLightContribution(HairSurface surface, Light light, int lightIndex, vec3 viewPos, vec3 V) {
    vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
    vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);

    vec3 lightVector = lightPos - WorldPos.xyz;
    float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
    vec3 L = lightVector * inversesqrt(distanceSquared);
    vec3 H = normalize(L + V);
    float attenuation = 1.0 / distanceSquared;

    float shadow = ShadowCalculationFast(lightIndex, lightPos, light.radius, WorldPos.xyz, viewPos, surface.normal, shadowMapArray);

    float dotTL = dot(surface.jitteredTangent, L);
    float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));
    vec3 diffuse = surface.hairBaseColor * sinTL;

    vec3 t1 = normalize(surface.jitteredTangent + surface.normal * 0.035);
    vec3 t2 = normalize(surface.jitteredTangent - surface.normal * 0.052);

    float D1 = HairSpecular(t1, H, surface.alpha1) * surface.specularMipFade;
    float D2 = HairSpecular(t2, H, surface.alpha2) * surface.specularMipFade;

    float dotVH = clamp(dot(V, H), 0.0, 1.0);
    float frensel = pow(1.0 - dotVH, 5.0);

    vec3 F1 = vec3(0.04) + vec3(0.96) * frensel;
    vec3 F2 = surface.hairBaseColor + (vec3(1.0) - surface.hairBaseColor) * frensel;

    vec3 spec1 = D1 * F1 * u_spec1Intensity;
    vec3 spec2 = D2 * F2 * surface.hairBaseColor * u_spec2Intensity;

    float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
    vec3 scattering = surface.hairBaseColor * scatterProp * u_scatterIntensity;

    vec3 lightContribution = (diffuse + spec1 + spec2 + scattering) * lightCol * light.strength;

    float hairWrapAmount = 0.75;
    float hairWrapPower = 1.0;
    float hairBackScatterStrength = 2.0;
    float hairBackScatterMin = 0.0;
    float hairBackScatterMax = 1.0;

    float nDotL = dot(surface.normal, L);
    float nDotLWrap = clamp((nDotL + hairWrapAmount) / (1.0 + hairWrapAmount), 0.0, 1.0);
    nDotLWrap = pow(nDotLWrap, hairWrapPower);
    nDotLWrap = mix(hairBackScatterMin, hairBackScatterMax, nDotLWrap);

    lightContribution *= nDotLWrap * hairBackScatterStrength;

#if ENABLE_BINDLESS == 1
    if (light.iesTextureIndex != 0) {
        sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
        lightContribution *= ApplyIESProfile(WorldPos.xyz, light, iesSampler);
    }
#endif

    return lightContribution * shadow * attenuation;
}

vec3 ComputeHairLighting(HairSurface surface) {
    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;
    vec3 V = normalize(viewPos - WorldPos.xyz);

    vec3 directLighting = vec3(0.0);

    for (int i = 2; i < 4; i++) {
        Light light = lights[i];
        directLighting += ComputeHairLightContribution(surface, light, i, viewPos, V);
    }

    return directLighting * surface.hairAO;
}

vec3 ComputeHairLightingCheap(HairSurface surface) {
    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;
    vec3 V = normalize(viewPos - WorldPos.xyz);

    vec3 directLighting = vec3(0.0);

    for (int i = 2; i < 4; i++) {
        Light light = lights[i];

        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightVector = lightPos - WorldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float dotTL = dot(surface.jitteredTangent, L);
        float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));

        float nDotL = dot(surface.normal, L);
        float nDotLWrap = clamp((nDotL + 0.75) / 1.75, 0.0, 1.0);

        vec3 diffuse = surface.hairBaseColor * sinTL;
        directLighting += diffuse * lightCol * light.strength * nDotLWrap * 2.0 / distanceSquared;
    }

    return directLighting * surface.hairAO;
}

uvec2 PackMLABColor(vec3 color, float alpha) {
    vec4 premultiplied = vec4(max(color * alpha, vec3(0.0)), clamp(alpha, 0.0, 1.0));

    return uvec2(
        packHalf2x16(premultiplied.rg),
        packHalf2x16(premultiplied.ba)
    );
}

uvec4 GetCurrentMLABNode(uint nodeIndex) {
    uvec4 node = nodes_v4[nodeIndex];

    if (node.w != u_mlabFrameIndex) {
        return uvec4(0u);
    }

    return node;
}

void WriteMLABNode(uint nodeIndex, uvec2 packedColor, uint depthUint) {
    nodes_v4[nodeIndex] = uvec4(packedColor.x, packedColor.y, depthUint, u_mlabFrameIndex);
}

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];

    HairTextureSamples samples = SampleHairTextures(renderItem);

    float hairMipLevelRaw = ComputeHairMipLevel(TexCoord, samples.baseTextureSizePixels);
    float coverageAlpha = ComputeHairCoverageAlphaFromMip(samples.baseColor.a, hairMipLevelRaw);

    if (coverageAlpha < 0.1 || coverageAlpha >= 0.5) {
        discard;
    }

    HairSurface surface = BuildHairSurface(samples, coverageAlpha, hairMipLevelRaw);

    vec3 color = ComputeHairLighting(surface);

    uvec2 packedColor = PackMLABColor(color, surface.alpha);
    uint depthUint = floatBitsToUint(gl_FragCoord.z);

    uint pixelIdx = uint(gl_FragCoord.y) * uint(u_renderTargetWidth) + uint(gl_FragCoord.x);
    uint nodeBase = pixelIdx * 4u;

    beginInvocationInterlockARB();

    uvec4 n0 = GetCurrentMLABNode(nodeBase + 0u);
    uvec4 n1 = GetCurrentMLABNode(nodeBase + 1u);
    uvec4 n2 = GetCurrentMLABNode(nodeBase + 2u);
    uvec4 n3 = GetCurrentMLABNode(nodeBase + 3u);

    uint d0 = n0.z;
    uint d1 = n1.z;
    uint d2 = n2.z;
    uint d3 = n3.z;

    if (depthUint > d3 || d3 == 0u) {
        if (depthUint > d0) {
            nodes_v4[nodeBase + 3u] = n2;
            nodes_v4[nodeBase + 2u] = n1;
            nodes_v4[nodeBase + 1u] = n0;
            WriteMLABNode(nodeBase + 0u, packedColor, depthUint);
        }
        else if (depthUint > d1) {
            nodes_v4[nodeBase + 3u] = n2;
            nodes_v4[nodeBase + 2u] = n1;
            WriteMLABNode(nodeBase + 1u, packedColor, depthUint);
        }
        else if (depthUint > d2) {
            nodes_v4[nodeBase + 3u] = n2;
            WriteMLABNode(nodeBase + 2u, packedColor, depthUint);
        }
        else {
            WriteMLABNode(nodeBase + 3u, packedColor, depthUint);
        }
    }

    endInvocationInterlockARB();
}