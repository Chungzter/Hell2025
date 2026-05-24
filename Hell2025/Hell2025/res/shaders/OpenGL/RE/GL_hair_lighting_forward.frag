#version 460 core

#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;
layout (binding = 9) uniform samplerCubeArray shadowMapArray;
layout (binding = 11) uniform sampler2D hairFlowMap;
layout (binding = 12) uniform sampler2D hairIdMap;
layout (binding = 13) uniform sampler2D hairRootMap;

layout(early_fragment_tests) in;

#include "../../common/hair.glsl"
#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer   { TileLights tileLights[]; };

layout (location = 0) out vec4 LightingOut;

centroid in vec2 v_texCoord;
centroid in vec3 v_normal;
centroid in vec3 v_tangent;
centroid in vec4 v_worldPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipv_normalMapY;
uniform float u_renderResolutionScale;

const float u_spec1Intensity   = 0.25;
const float u_spec2Intensity   = 0.9;
const float u_scatterPower     = 12.0;
const float u_scatterIntensity = 0.1;
const float u_specularJitter   = 0.0;
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





void ComputeAlphaToCoverage(float alpha) {
    vec2 v_uv = v_texCoord;                       // TODO: just rename the actual varying
    vec2 textureSizePixels = vec2(1920, 1080);  // TODO: do not hardcode this!!!

    vec2 dx = dFdx(v_uv) * textureSizePixels;
    vec2 dy = dFdy(v_uv) * textureSizePixels;
    float mipLevel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));

    float alphaPivot = 0.025;
    float alphaSharpness = 0.75;
    float alphaBaseBoost = 1.0;

    float boost = max(alphaBaseBoost, mipLevel * alphaSharpness);
    alpha = clamp((alpha - alphaPivot) * boost + alphaPivot, 0.0, 1.0);

    if (alpha <= 0.10) {
        return;
    }

    uint mask =
        (uint(alpha > 0.10) << 0) |
        (uint(alpha > 0.35) << 1) |
        (uint(alpha > 0.65) << 2) |
        (uint(alpha > 0.90) << 3);

    gl_SampleMask[0] = int(mask);
}

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];

    sampler2D baseColorSampler = sampler2D(textureSamplers[renderItem.baseColorTextureIndex]);

    vec4 baseColor = texture(baseColorSampler, v_texCoord);
    vec4 rma = texture(sampler2D(textureSamplers[renderItem.rmaTextureIndex]), v_texCoord).rgba;
    vec4 additionalColor0 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex0]), v_texCoord);
    vec4 additionalColor1 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex1]), v_texCoord);
    vec4 additionalColor2 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex2]), v_texCoord);
    vec2 baseTextureSizePixels = vec2(textureSize(baseColorSampler, 0));

    // TODO: sample alpha from opacity texture instead of basecolor.a and return early if empty at top of this shader
    //ComputeAlphaToCoverage(baseColor.a);

    float hairMipLevelRaw = ComputeHairMipLevel(v_texCoord, baseTextureSizePixels);
    float coverageAlpha = ComputeHairCoverageAlphaFromMip(baseColor.a, hairMipLevelRaw);

    //if (coverageAlpha < 0.5) {
    //    discard;
    //}

    baseColor.a = coverageAlpha;

    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;

    vec3 flowSample = additionalColor0.rgb;
    vec2 flow = flowSample.rg * 2.0 - 1.0;
    float strandID = additionalColor1.r;
    float rootFactor = additionalColor2.r;

    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;
    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec3 V = normalize(viewPos - v_worldPos.xyz);
    vec3 n = normalize(v_normal);

    if (!gl_FrontFacing) {
        n = -n;
    }

    vec3 t_mesh = normalize(v_tangent);
    t_mesh = normalize(t_mesh - dot(t_mesh, n) * n);
    vec3 b_mesh = cross(n, t_mesh);

    vec3 t = BuildHairTangentFromFlow(t_mesh, b_mesh, flow);
    vec3 jitteredT = SafeNormalize(t + n * (strandID - 0.5) * u_specularJitter, t);

    vec3 hairBaseColor = gammaBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    float hairAO = ao * mix(u_rootAOFloor, u_tipAOFloor, rootFactor);

    const float u_specularAARoughnessPerMip = 0.5;
    const float u_specularMipFadeStrength = 0.3;
    const float u_specularAlpha1Min = 0.055;
    const float u_specularAlpha2Min = 0.070;
    const float u_specularMipStart = 0.9;

    float mipLevelRaw = max(0.0, hairMipLevelRaw + log2(u_renderResolutionScale));
    float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);

    float roughnessAA = clamp(roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);
    float specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

    float ue4Roughness = mix(0.15, 0.25, roughnessAA);

    float alpha1 = clamp(ue4Roughness * ue4Roughness, u_specularAlpha1Min, 1.0);
    float alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, u_specularAlpha2Min, 1.0);

    vec3 t1 = normalize(jitteredT + n * 0.035);
    vec3 t2 = normalize(jitteredT - n * 0.052);

    vec3 directLighting = vec3(0.0);

    for (int i = 2; i < 4; i++) {
        Light light = lights[i];

        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        vec3 L = lightVector * inversesqrt(distanceSquared);
        vec3 H = normalize(L + V);
        float attenuation = 1.0 / distanceSquared;

        float shadow = ShadowCalculationFast(i, lightPos, light.radius, v_worldPos.xyz, viewPos, n, shadowMapArray);

        float dotTL = dot(jitteredT, L);
        float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));
        vec3 diffuse = hairBaseColor * sinTL;

        float D1 = HairSpecular(t1, H, alpha1) * specularMipFade;
        float D2 = HairSpecular(t2, H, alpha2) * specularMipFade;

        float dotVH = clamp(dot(V, H), 0.0, 1.0);
        float frensel = pow(1.0 - dotVH, 5.0);

        vec3 F1 = vec3(0.04) + vec3(0.96) * frensel;
        vec3 F2 = hairBaseColor + (vec3(1.0) - hairBaseColor) * frensel;

        vec3 spec1 = D1 * F1 * u_spec1Intensity;
        vec3 spec2 = D2 * F2 * hairBaseColor * u_spec2Intensity;

        float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
        vec3 scattering = hairBaseColor * scatterProp * u_scatterIntensity;

        vec3 lightContribution = (diffuse + spec1 + spec2 + scattering) * lightCol * light.strength;

        float hairWrapAmount = 0.75;
        float hairWrapPower = 1.0;
        float hairBackScatterStrength = 2.0;
        float hairBackScatterMin = 0.0;
        float hairBackScatterMax = 1.0;

        float nDotL = dot(n, L);
        float nDotL_wrap = clamp((nDotL + hairWrapAmount) / (1.0 + hairWrapAmount), 0.0, 1.0);
        nDotL_wrap = pow(nDotL_wrap, hairWrapPower);
        nDotL_wrap = mix(hairBackScatterMin, hairBackScatterMax, nDotL_wrap);

        lightContribution *= nDotL_wrap * hairBackScatterStrength;

#if ENABLE_BINDLESS == 1
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        }
#endif
        directLighting += lightContribution * shadow * attenuation;
    }


    vec3 color = directLighting * hairAO;

    LightingOut = vec4(color, 1.0);

}