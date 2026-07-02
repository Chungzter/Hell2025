#version 460

#extension GL_ARB_bindless_texture : enable

#include "../../common/gl_fixed_bindings.glsl"
#include "../../common/constants.glsl"
#include "../../common/lighting.glsl"
#include "../../common/distance_fog.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 LightingOut;

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray u_flashlighShadowMapArrayTexture;
layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow u_hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow u_lowResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_CSM)        uniform sampler2DArray u_shadowMapCascadeArray;

layout (binding = 4) uniform sampler2D u_baseColorMetallicTexture;
layout (binding = 5) uniform sampler2D u_normalXYRoughnessMiscTexture;
layout (binding = 6) uniform sampler2D u_velocityXYOcclusionSubSurfaceTexture;
layout (binding = 7) uniform sampler2D u_depthTexture;
layout (binding = 8) uniform sampler2D u_indirectDiffuseTexture;
layout (binding = 9) uniform sampler2D u_flashlightCookieTexture;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer   { TileLights tileLights[];   };

// Moon lighting
uniform float u_cascadeFarPlane = 256.0;
uniform float u_cascadePlaneDistances[16];
#include "../../common/moon_lighting.glsl"

// TODO: dont hardcode
uniform float u_oceanHeight = 30;

void main() {
	ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uint viewportIndex = ComputeViewportIndexFromSplitscreenMode(px, outputImageSize, rendererData.splitscreenMode);
    vec2 screenUV = (vec2(px) + 0.5) / vec2(outputImageSize);
    vec2 viewportUV = ScreenUVToViewportUV(screenUV, viewportDataArr[viewportIndex]);

    vec4 normalXYRoughnessMisc = texelFetch(u_normalXYRoughnessMiscTexture, px, 0);
    vec3 normal = DecodeNormal(normalXYRoughnessMisc.rg);
    float roughness = normalXYRoughnessMisc.b;
    float misc = normalXYRoughnessMisc.a;

    vec4 baseColorMetallic = texelFetch(u_baseColorMetallicTexture, px, 0);
    vec3 baseColor = baseColorMetallic.rgb;
    float metallic = baseColorMetallic.a;
    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2)); // baseColor.rgb * baseColor.rgb;

    vec4 velocityXYOcclusionSubSurface = texelFetch(u_velocityXYOcclusionSubSurfaceTexture, px, 0).rgba;
    float ao =velocityXYOcclusionSubSurface.b;
    float subSurface = velocityXYOcclusionSubSurface.a;
    vec2 velocity = velocityXYOcclusionSubSurface.rg;

    ViewportData viewportData = viewportDataArr[viewportIndex];
    mat4 inverseProjection = viewportData.inverseProjection;
    mat4 inverseProjectionViewReverseZ = viewportData.inverseProjectionViewReverseZ;
    mat4 inverseView = viewportData.inverseView;
    mat4 viewMatrix = viewportData.view;
    vec3 viewPos = viewportData.viewPos.xyz;
    bool thisViewportIsInShop = bool(viewportData.isInShop);

    // Tile data
    uvec2 tileCoord = uvec2(px) / uint(TILE_SIZE);
    uint tileIndex = tileCoord.y * rendererData.tileCountX + tileCoord.x;
	uint lightCount = tileLights[tileIndex].lightCount;

    // Depth reconstruction
    float depth = texelFetch(u_depthTexture, px, 0).r;
    vec3 worldPos = ReconstructWorldPos(viewportUV, depth, inverseProjectionViewReverseZ);

    float fragDistance = distance(worldPos, viewPos);

    vec3 F0 = mix(vec3(0.04), linearBaseColor, metallic);



    vec3 directLighting = vec3(0.0);

    for (int i = 0; i < lightCount; i++) {
        int lightIndex = int(tileLights[tileIndex].lightIndices[i]);

        Light light = lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationSkin(light.hiResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, normal.xyz, u_hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationSkin(light.lowResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, normal.xyz, u_lowResShadowMapArray);
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * shadow;

        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
            float candelas = ApplyIESProfile(worldPos.xyz, light, iesSampler);
            directLight *= candelas;
        }

        directLighting += directLight;
    }


    for (int i = 0; i < 2; i++) {
        ViewportData flashlightViewportData = viewportDataArr[i];
        directLighting += GetFlashlightContribution(i, viewportIndex, flashlightViewportData.flashlightModifer, flashlightViewportData.flashlightProjectionView, flashlightViewportData.flashlightDir.xyz, flashlightViewportData.flashlightPosition.xyz, flashlightViewportData.inverseView[3].xyz, bool(flashlightViewportData.isInShop), GetFlashLightColor(), normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, u_oceanHeight, u_flashlightCookieTexture, u_flashlighShadowMapArrayTexture);
    }

    vec3 indirectDiffuse = vec3(0);
    bool u_sampleProbes = true;
    if (u_sampleProbes) {
        vec3 probeIrradiance = texture(u_indirectDiffuseTexture, screenUV).rgb;
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        indirectDiffuse = probeIrradiance * diffuseAlbedo;
    }

    // Extra flashlight lighting
    if (viewportDataArr[viewportIndex].flashlightModifer > 0.1) {
        vec3 offset = viewportDataArr[viewportIndex].cameraForward.xyz * 0.001;
        vec3 spotLightPos = viewPos + offset;
        vec3 spotLightDir = viewportDataArr[viewportIndex].flashlightDir.xyz;
        vec3 spotLightColor = vec3(0.9, 0.95, 1.1);

        spotLightColor = mix(spotLightColor, vec3(1, 0.7799999713897705, 0.5289999842643738), 0.5);

        float spotLightRadius = 0.165;
        float spotLightStregth = 10.0;
        float innerAngle = cos(radians(00.0 * viewportDataArr[viewportIndex].flashlightModifer));
        float outerAngle = cos(radians(40.0));

        mat4 lightProjectionView = viewportDataArr[viewportIndex].flashlightProjectionView;
        vec4 flashlightDir = viewportDataArr[viewportIndex].flashlightDir;
        vec4 flashlightPosition = viewportDataArr[viewportIndex].flashlightPosition;
        vec3 flashlightViewPos = viewportDataArr[viewportIndex].inverseView[3].xyz;

        vec3 re7Lighting = GetSpotlightLighting(spotLightPos, spotLightDir, spotLightColor, spotLightRadius, spotLightStregth, innerAngle, outerAngle, normal.xyz, worldPos, linearBaseColor.rgb, roughness, metallic, flashlightViewPos, lightProjectionView);
        directLighting += re7Lighting;
    }

    // Moon light
    vec3 moonLighting = vec3(0.0);
    vec3 moonLightDir = rendererData.moonLightDir.xyz;
    float moonNdotL = dot(normal.xyz, moonLightDir);

    if (moonNdotL > 0.0) {
        vec3 shadow = ShadowCalculationCSM(worldPos, normal.xyz, moonLightDir, viewMatrix, viewportIndex);
    
        if (any(greaterThan(shadow, vec3(0.0)))) {
            moonLighting = GetDirectionalLighting(moonLightDir, MOON_LIGHT_COLOR, MOON_LIGHT_STRENGTH, normal.xyz, worldPos, linearBaseColor.rgb, roughness, metallic, viewPos);
            moonLighting *= shadow;
        }
    }

    vec3 finalColor = (directLighting + indirectDiffuse + moonLighting) * ao;

    // Distance Fog
    finalColor = DistanceFog(finalColor, fragDistance);
    
    LightingOut = vec4(finalColor, 1);
}
