#version 460

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
layout (binding = 12) uniform sampler2D hairIDMap;
layout (binding = 13) uniform sampler2D hairRootMap;

#include "../../common/lighting.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/post_processing.glsl"

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer   { TileLights tileLights[];   };

layout (location = 0) out vec4 LightingOut;
layout (location = 1) out vec4 BaseColorOut;
layout (location = 2) out vec4 NormalOut;
layout (location = 3) out vec4 MaterialOut;
layout (location = 4) out vec4 RENormalOut;

centroid in vec2 TexCoord;
centroid in vec3 Normal;
centroid in vec3 Tangent;
centroid in vec4 WorldPos;
centroid in vec3 ViewPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipNormalMapY;

void main() {
    // grab the item once to avoid multiple buffer lookups
    RenderItem item = renderItems[v_globalInstanceIndex];

#if ENABLE_BINDLESS == 1
    vec4 baseColor = texture(sampler2D(textureSamplers[item.baseColorTextureIndex]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[item.normalMapTextureIndex]), TexCoord).rgb;
    vec4 rma = texture(sampler2D(textureSamplers[item.rmaTextureIndex]), TexCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[item.emissiveTextureIndex]), TexCoord).rgb;
    
    // these were being fetched but not used in the lighting loop yet
    vec3 additionalTex0 = texture(sampler2D(textureSamplers[item.additionalTextureIndex0]), TexCoord).rgb;
    vec3 additionalTex1 = texture(sampler2D(textureSamplers[item.additionalTextureIndex1]), TexCoord).rgb;
    vec3 additionalTex2 = texture(sampler2D(textureSamplers[item.additionalTextureIndex2]), TexCoord).rgb;
#else
    vec4 baseColor = texture(baseColorTexture, TexCoord);
    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    vec4 rma = texture(rmaTexture, TexCoord).rgba;
    vec3 emissiveMapColor = texture(emissiveTexture, TexCoord).rgb;
#endif

    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;

    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;

    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    ViewportData vd = viewportData[v_viewportIndex];
    mat4 inverseProjection = vd.inverseProjection;
    mat4 inverseView = vd.inverseView;
    mat4 viewMatrix = vd.view;
    bool thisViewportIsInShop = bool(vd.isInShop);

    // normal mapping
    normalMap = normalMap * 2.0 - 1.0;
    vec3 n = normalize(Normal);
    vec3 t = normalize(Tangent);
    if (!gl_FrontFacing) n = -n;
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);
    vec3 normal = normalize(tbn * normalMap);

    // Material out
    //vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));
    vec3 linearBaseColor = baseColor.rgb * baseColor.rgb;
    vec3 F0 = mix(vec3(0.04), linearBaseColor, metallic);
    float f0_lum = dot(F0, vec3(0.2126, 0.7152, 0.0722));
    MaterialOut = vec4(roughness, metallic, ao, f0_lum);

    // Fix fireflies
    //float geometricRoughness = length(fwidth(normal));
    //roughness = clamp(roughness + geometricRoughness, 0.0, 1.0);
    //
    //float filterRadius = 0.1;
    //float geometricRoughness = length(fwidth(normal));
    //roughness = max(roughness, geometricRoughness * filterRadius);

    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5; 
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    //float normalMapVariation = length(fwidth(normalMap.rgb));
    //roughness = clamp(roughness + (normalMapVariation * 0.5), 0.0, 1.0);

    vec3 directLighting = vec3(0.0);
    for (int i = 2; i < 4; i++) {
        int lightIndex = i; //int(tileLights[tileIndex].lightIndices[i]);

        Light light = lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        float shadow = ShadowCalculation(lightIndex, lightPosition, lightRadius, WorldPos.xyz, viewPos, normal.xyz, shadowMapArray);
        vec3 directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, viewPos) * shadow;

        #if ENABLE_BINDLESS == 1
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
            float candelas = ApplyIESProfile(WorldPos.xyz, light, iesSampler);
            directLight *= candelas;
        }
        #endif

        directLighting += directLight;
    }

    vec3 finalLitColor = directLighting * ao;
    
    LightingOut.rgb = finalLitColor;
    LightingOut.a = baseColor.a;
    
    NormalOut = vec4(normalize(normal) * 0.5 + 0.5, 1.0);

    RENormalOut.rg = EncodeNormal(normal);
    RENormalOut.b = metallic;
    RENormalOut.a = 0.0; // Misc 4 bit value

    //RENormalOut = vec4(1,0,0,1);

    BaseColorOut = baseColor;

    
    //LightingOut.rgba = vec4(baseColor.rgb, 0);
}