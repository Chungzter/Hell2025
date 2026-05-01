#version 460 core

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#if ENABLE_BINDLESS == 1
    #extension GL_ARB_bindless_texture : enable
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

#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer   { TileLights tileLights[];   };

layout (location = 0) out vec4 ColorOut;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 MaterialOut;

centroid in vec2 TexCoord;
centroid in vec3 Normal;
centroid in vec3 Tangent;
centroid in vec4 WorldPos;
centroid in vec3 ViewPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipNormalMapY;

const float u_spec1Intensity   = 0.2;
const float u_spec2Intensity   = 0.4;
const float u_scatterPower     = 12.0;  
const float u_scatterIntensity = 0.1;   
const float u_specularJitter   = 0.0;
const float u_rootColorFloor   = 0.2;  
const float u_rootAOFloor      = 0.7;   
const float u_tipColorFloor    = 0.845;
const float u_tipAOFloor       = 0.7;

// UE4 kind of anisotropic hair specular evaluation
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

float GetTextureMipLevel(vec2 uv, vec2 textureSizePixels) {
    vec2 uvTexels = uv * textureSizePixels;

    vec2 dx = dFdx(uvTexels);
    vec2 dy = dFdy(uvTexels);

    return max(0.0, 0.5 * log2(max(dot(dx, dx), dot(dy, dy))));
}

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];

#if ENABLE_BINDLESS == 1
    vec4 baseColor = texture(sampler2D(textureSamplers[renderItem.baseColorTextureIndex]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[renderItem.normalMapTextureIndex]), TexCoord).rgb;
    vec4 rma = texture(sampler2D(textureSamplers[renderItem.rmaTextureIndex]), TexCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[renderItem.emissiveTextureIndex]), TexCoord).rgb;
    vec4 additionalColor0 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex0]), TexCoord);
    vec4 additionalColor1 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex1]), TexCoord);
    vec4 additionalColor2 = texture(sampler2D(textureSamplers[renderItem.additionalTextureIndex2]), TexCoord);
#else
    vec4 baseColor = texture(baseColorTexture, TexCoord);
    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    vec4 rma = texture(rmaTexture, TexCoord).rgba;
    vec3 emissiveMapColor = texture(emissiveTexture, TexCoord).rgb;
    vec4 additionalColor0 = texture(hairFlowMap, TexCoord);
    vec4 additionalColor1 = texture(hairIdMap, TexCoord);
    vec4 additionalColor2 = texture(hairRootMap, TexCoord);
#endif

    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;

    vec3 flowSample = additionalColor0.rgb;
    vec2 flow = flowSample.rg * 2.0 - 1.0;
    float strandID = additionalColor1.r;
    float rootFactor = additionalColor2.r;

    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;
    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec3 V = normalize(viewPos - WorldPos.xyz);
    vec3 n = normalize(Normal);
    if (!gl_FrontFacing) n = -n;

    vec3 t_mesh = normalize(Tangent);
    t_mesh = normalize(t_mesh - dot(t_mesh, n) * n);
    vec3 b_mesh = cross(n, t_mesh);

    // Apply flow map direction to tangent
    //vec3 t = normalize(t_mesh * flow.x + b_mesh * flow.y);
    //vec3 jitteredT = normalize(t + n * (strandID - 0.5) * u_specularJitter);
    vec3 t = BuildHairTangentFromFlow(t_mesh, b_mesh, flow);
    vec3 jitteredT = SafeNormalize(t + n * (strandID - 0.5) * u_specularJitter, t);
    
    // Clamp base color at roots
    vec3 hairBaseColor = gammaBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    float hairAO = ao * mix(u_rootAOFloor, u_tipAOFloor, rootFactor);

    vec3 directLighting = vec3(0.0);
    
    for (int i = 0; i < 6; i++) {
        //if (i != 2) continue;

        Light light = lights[i];
        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 L = normalize(lightPos - WorldPos.xyz);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);
        vec3 H = normalize(L + V);
        
        float shadow = ShadowCalculation(i, lightPos, light.radius, WorldPos.xyz, viewPos, n, shadowMapArray);

        // Kajiya diffuse
        float dotTL = dot(jitteredT, L);
        float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));
        vec3 diffuse = hairBaseColor * sinTL;

        // Cuticle shifts based on standard physical profile
        vec3 t1 = normalize(jitteredT + n * 0.035);
        vec3 t2 = normalize(jitteredT - n * 0.052);

        //float ue4Roughness = mix(0.15, 0.25, roughness); 
        //float alpha1 = clamp(ue4Roughness * ue4Roughness, 0.01, 1.0);
        //float alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, 0.01, 1.0);
        //
        //float D1 = HairSpecular(t1, H, alpha1);
        //float D2 = HairSpecular(t2, H, alpha2);

        const float u_specularAARoughnessPerMip = 0.5;  // bigger is blurrier with distance
        const float u_specularMipFadeStrength = 0.3;    // bigger is dimmer with distance
        const float u_specularAlpha1Min = 0.055;        // bigger is wider/softer primary highlight
        const float u_specularAlpha2Min = 0.070;        // bigger is wider/softer secondary highlight
        const float u_specularMipStart = 0.9;           // bigger is delay distance blur

        vec2 baseTextureSizePixels = vec2(textureSize(sampler2D(textureSamplers[renderItem.baseColorTextureIndex]), 0));
        float mipLevelRaw = GetTextureMipLevel(TexCoord, baseTextureSizePixels);
        float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);

        float roughnessAA = clamp(roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);
        float specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

        float ue4Roughness = mix(0.15, 0.25, roughnessAA);

        float alpha1 = clamp(ue4Roughness * ue4Roughness, u_specularAlpha1Min, 1.0);
        float alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, u_specularAlpha2Min, 1.0);

        float D1 = HairSpecular(t1, H, alpha1) * specularMipFade;
        float D2 = HairSpecular(t2, H, alpha2) * specularMipFade;

        float dotVH = clamp(dot(V, H), 0.0, 1.0);
        vec3 F1 = vec3(0.04) + vec3(0.96) * pow(1.0 - dotVH, 5.0);
        vec3 F2 = hairBaseColor + (vec3(1.0) - hairBaseColor) * pow(1.0 - dotVH, 5.0);

        // Apply intensity constants
        vec3 spec1 = D1 * F1 * u_spec1Intensity;
        vec3 spec2 = D2 * F2 * hairBaseColor * u_spec2Intensity;

        float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
        vec3 scattering = hairBaseColor * scatterProp * u_scatterIntensity;
        vec3 lightContribution = (diffuse + spec1 + spec2 + scattering) * lightCol * light.strength;
        
        //float nDotL_wrap = clamp((dot(n, L) + 0.5) / 0.5, 0.0, 1.0);
        //lightContribution *= nDotL_wrap;
        

        float dist = length(lightPos - WorldPos.xyz);
        float attenuation = 1.0 / (dist * dist);
        
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
            lightContribution *= ApplyIESProfile(WorldPos.xyz, light, iesSampler);
        }

        directLighting += lightContribution * shadow * attenuation;
    }

    ColorOut = vec4(directLighting * hairAO, baseColor.a);
    MaterialOut = vec4(roughness, metallic, hairAO, 1.0);
    NormalOut = vec4(normalize(n) * 0.5 + 0.5, 1.0);
}