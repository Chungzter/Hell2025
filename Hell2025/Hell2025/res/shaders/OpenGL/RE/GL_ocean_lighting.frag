#version 450 core
#extension GL_ARB_bindless_texture : enable
#include "../../common/lighting.glsl"
#include "../../common/ocean.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 ColorOut;
layout (location = 1) out uint OceanMaskOut;

layout (binding = 0) uniform sampler2D DisplacementTexture_band0;
layout (binding = 1) uniform sampler2D SlopeTexture_band0;
layout (binding = 2) uniform sampler2D DisplacementTexture_band1;
layout (binding = 3) uniform sampler2D SlopeTexture_band1;
layout (binding = 4) uniform samplerCube cubeMap;
layout (binding = 5) uniform sampler2D DetailRippleNormal;
layout (binding = 6) uniform sampler2DArray u_flashlighShadowMapArrayTexture;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 2) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

in vec3 v_worldPos;

uniform int u_viewportIndex;
uniform float u_oceanOriginY;
uniform float u_time;

uniform int u_displayMode = OCEAN_DISPLAY_MODE_COMBINED;

struct OceanSurfaceSettings {
    bool specularAntiAliasing;
    vec3 albedo;
    vec3 fogColor;
    float normalScale;
    float normalConvergeStartDistance;
    float normalConvergeEndDistance;
    float normalConvergeMaxFactor;
    float normalConvergeExponent;
    float normalSoftening;
    float rippleTiling;
    float rippleStrength;
    float rippleSecondLayerScale;
    vec2 rippleVelocity0;
    vec2 rippleVelocity1;
    float roughness;
    float reflectance;
    float reflectionGamma;
    float diffuseStrength;
    float sssHeightRange;
    float sssStrength;
    float underwaterSssStrength;
    float sssRadiusMinimum;
    float sssRadiusMaximum;
    float sssIntensity;
    float sssFalloff;
    float sssSaturation;
    float fogStartDistance;
    float fogEndDistance;
    float fogExponent;
    float fogStrength;
};

uniform OceanSurfaceSettings u_surface;

// -90 degrees Y rotation (no cos/sin per fragment)
const mat3 kRotateYMinus90 = mat3(0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0);

void main() {
    vec3 viewPos = viewportDataArr[u_viewportIndex].viewPos.xyz;

    vec3 WorldPos = v_worldPos;

    vec2 worldXZ = WorldPos.xz;

    float viewDist = length(WorldPos - viewPos);
    vec2 worldDdx = dFdx(worldXZ);
    vec2 worldDdy = dFdy(worldXZ);
    float lod0 = OceanNormalLod(worldDdx, worldDdy, 0);
    float lod1 = OceanNormalLod(worldDdx, worldDdy, 1);
    vec2 normalXZ = SampleCombinedEstimatedOceanNormalXZ(DisplacementTexture_band0, SlopeTexture_band0, DisplacementTexture_band1, SlopeTexture_band1, worldXZ, lod0, lod1, u_displayMode);

    normalXZ *= u_surface.normalScale;
    vec3 normal = normalize(vec3(normalXZ.x, 1.0, normalXZ.y));

    // Converge to up normal over distance
    float t2 = clamp((viewDist - u_surface.normalConvergeStartDistance) / (u_surface.normalConvergeEndDistance - u_surface.normalConvergeStartDistance), 0.0, 1.0);
    t2 = pow(t2, u_surface.normalConvergeExponent) * u_surface.normalConvergeMaxFactor;
    normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), t2));

    // Aggresively reduce normals
    normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), u_surface.normalSoftening));


    // High frequency normal ripples
    vec2 rippleUV1 = worldXZ * u_surface.rippleTiling + u_time * u_surface.rippleVelocity0;
    vec2 rippleUV2 = worldXZ * (u_surface.rippleTiling * u_surface.rippleSecondLayerScale) + u_time * u_surface.rippleVelocity1;
    vec3 ripple1 = texture(DetailRippleNormal, rippleUV1).xyz * 2.0 - 1.0;
    vec3 ripple2 = texture(DetailRippleNormal, rippleUV2).xyz * 2.0 - 1.0;
    vec3 microRipplesTangent = ripple1 + ripple2;
    vec3 rippleWorld = vec3(microRipplesTangent.x, 0.0, microRipplesTangent.y);
    normal = normal + (rippleWorld * u_surface.rippleStrength);

    vec3 N = normalize(normal);
    vec3 V_view = normalize(viewPos - WorldPos);

    if (dot(N, V_view) < 0.0) {
        V_view = -V_view;
    }

    float roughness = u_surface.roughness;
    // Widen highlights when the normal moves faster than the pixels can represent
    if (u_surface.specularAntiAliasing) {
        vec3 normalDdx = dFdx(N);
        vec3 normalDdy = dFdy(N);
        float normalVariance = 0.15 * (dot(normalDdx, normalDdx) + dot(normalDdy, normalDdy));
        float kernelRoughness = min(2.0 * normalVariance, 0.18);
        roughness = min(sqrt(roughness * roughness + kernelRoughness), 1.0);
    }
    vec3 F0 = vec3(u_surface.reflectance);

    // SSS height
    float h = WorldPos.y - u_oceanOriginY;
    float u_minHeight = u_oceanOriginY - u_surface.sssHeightRange;
    float u_maxHeight = u_oceanOriginY + u_surface.sssHeightRange;
    float hNorm = clamp((h - u_minHeight) / (u_maxHeight - u_minHeight), 0.0, 1.0);

    vec3 surfaceLighting = vec3(0.0);

    // Moonlight
    vec3 moonColor = GetMoonLightColor();
    vec3 moonLightDir = rendererData.moonLightDir.xyz;
    vec3 L = moonLightDir;
    if (!gl_FrontFacing) L.x *= -1;
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    vec3 spec_direct = microfacetSpecular(L, V_view, N, F0, roughness);
    vec3 Lo_direct = spec_direct * moonColor * NoL;
    vec3 R = reflect(-V_view, N);
    vec3 R_rotated = kRotateYMinus90 * R;
    vec3 kS_IBL = fresnelSchlick(clamp(dot(N, V_view), 0.0, 1.0), F0);

    float reflectionLod = 0.0;
    if (u_surface.specularAntiAliasing) {
        vec3 reflectionDdx = dFdx(R_rotated);
        vec3 reflectionDdy = dFdy(R_rotated);
        float reflectionFootprintSq = max(dot(reflectionDdx, reflectionDdx), dot(reflectionDdy, reflectionDdy));
        float cubeMapSize = float(textureSize(cubeMap, 0).x);
        float maxReflectionLod = float(textureQueryLevels(cubeMap) - 1);

        // Blur reflections once their normal footprint is bigger than a cubemap texel
        float footprintLod = 0.5 * log2(max(reflectionFootprintSq * cubeMapSize * cubeMapSize, 1.0));
        float roughnessLod = roughness * maxReflectionLod;
        reflectionLod = clamp(max(footprintLod, roughnessLod), 0.0, maxReflectionLod);
    }

    vec3 reflection_IBL = textureLod(cubeMap, R_rotated, reflectionLod).rgb;
    reflection_IBL = pow(reflection_IBL, vec3(u_surface.reflectionGamma));
    vec3 specular_IBL = reflection_IBL * kS_IBL;
    vec3 diffuse_IBL = moonColor * u_surface.albedo * u_surface.diffuseStrength;

    surfaceLighting += Lo_direct;
    surfaceLighting += diffuse_IBL;
    surfaceLighting += specular_IBL;

    if (rendererData.flashlightIESTextureIndex >= 0) {
        sampler2D flashlightIES = sampler2D(textureSamplers[rendererData.flashlightIESTextureIndex]);
        for (int i = 0; i < 2; i++) {
            ViewportData flashlightViewportData = viewportDataArr[i];
            surfaceLighting += GetFlashlightContribution(i, uint(u_viewportIndex), flashlightViewportData.flashlightModifer, flashlightViewportData.flashlightProjectionView, flashlightViewportData.flashlightDir.xyz, flashlightViewportData.flashlightPosition.xyz, flashlightViewportData.inverseView[3].xyz, bool(flashlightViewportData.isInShop), rendererData, N, WorldPos, u_surface.albedo, roughness, 0.0, viewDist, u_oceanOriginY, flashlightIES, u_flashlighShadowMapArrayTexture);
        }
    }

    // SSS
    float sssStrength = gl_FrontFacing ? u_surface.sssStrength : u_surface.underwaterSssStrength;
    float sssRadius = mix(u_surface.sssRadiusMinimum, u_surface.sssRadiusMaximum, hNorm);
    float NdotL = max(dot(N, L), 0.0);
    float sssScalar = u_surface.sssIntensity * exp(-u_surface.sssFalloff * abs(NdotL) / (sssRadius + 0.001));
    vec3 subColor = Saturate(u_surface.albedo, u_surface.sssSaturation);
    vec3 sssColor = subColor * sssRadius * sssScalar * sssStrength;

    surfaceLighting += sssColor;


    // Fog (reuse viewDist)
    {
        float fogRange = u_surface.fogEndDistance - u_surface.fogStartDistance;
        float normDist = (viewDist - u_surface.fogStartDistance) / max(fogRange, 0.0001);
        normDist = clamp(normDist, 0.0, 1.0);

        float fogEffect = pow(normDist, u_surface.fogExponent);
        float fogFactor = 1.0 - fogEffect;

        surfaceLighting = mix(u_surface.fogColor * u_surface.fogStrength, surfaceLighting, fogFactor);
    }

    ColorOut = vec4(surfaceLighting, 1.0);

    if (gl_FrontFacing) {
        OceanMaskOut = 1u; // Top side
    }
    else {
        OceanMaskOut = 2u; // Bottom side
    }
}
