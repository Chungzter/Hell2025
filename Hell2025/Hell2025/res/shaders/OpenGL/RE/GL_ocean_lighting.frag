#version 450 core

#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 ColorOut;
layout (location = 1) out uint OceanMaskOut;

layout (binding = 0) uniform sampler2D DisplacementTexture_band0;
layout (binding = 1) uniform sampler2D NormalTexture_band0;
layout (binding = 2) uniform sampler2D DisplacementTexture_band1;
layout (binding = 3) uniform sampler2D NormalTexture_band1;
layout (binding = 4) uniform samplerCube cubeMap;
layout (binding = 5) uniform sampler2D DetailRippleNormal;

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

in vec3 v_worldPos;
in vec3 v_normal;
in vec3 v_debugColor;

uniform int u_viewportIndex;
uniform float u_oceanOriginY;
uniform float u_time;

const float WATER_HEIGHT = 29.5;
const float u_nearMipDist = 50.0;
const float u_farMipDist = 100.0;
const float u_maxMipLevel = 4.0;
uniform int u_mode = 0;

// -90 degrees Y rotation (no cos/sin per fragment)
const mat3 kRotateYMinus90 = mat3(
    0.0, 0.0, -1.0,
    0.0, 1.0,  0.0,
    1.0, 0.0,  0.0
);

vec3 SampleEstimatedNormalBand(sampler2D displacementTex, sampler2D normalTex, vec2 worldXZ, float invPatchSize, float patchSize, float displacementScale, float lod) {
    vec2 bestGuessUV = worldXZ * invPatchSize;

    vec3 disp = texture(displacementTex, bestGuessUV).xyz;
    vec2 estimatedDisplacement = disp.xz * displacementScale;

    vec2 estimatedWorldPosition = worldXZ - estimatedDisplacement;
    vec2 estimatedUV = estimatedWorldPosition / patchSize;

    return textureLod(normalTex, estimatedUV, lod).xyz;
}

void main() {
    vec3 viewPos = viewportDataArr[u_viewportIndex].viewPos.xyz;

    vec3 WorldPos = v_worldPos;
    vec3 u_fogColor = vec3(0.00326, 0.00217, 0.00073);

    // Band constants
    const float fftResoltion_band0 = 512.0;
    const float fftResoltion_band1 = 512.0;
    const float patchSize_band0 = 8.0;
    const float patchSize_band1 = 13.123;

    const float invPatchSize_band0 = 1.0 / patchSize_band0;
    const float invPatchSize_band1 = 1.0 / patchSize_band1;

    const float displacementScale_band0 = patchSize_band0 / fftResoltion_band0;
    const float displacementScale_band1 = patchSize_band1 / fftResoltion_band1;

    vec2 worldXZ = WorldPos.xz;

    float viewDist = length(WorldPos - viewPos);
    vec2 ddx = dFdx(worldXZ);
    vec2 ddy = dFdy(worldXZ);
    
    // calculating squared length to avoid sqrt
    float footprintSq = max(dot(ddx, ddx), dot(ddy, ddy));
    float log2FootprintHalf = 0.5 * log2(footprintSq);

    // caching log2 texel sizes to avoid division
    const float log2TexelSize_band0 = log2(patchSize_band0 / fftResoltion_band0);
    const float log2TexelSize_band1 = log2(patchSize_band1 / fftResoltion_band1);

    // computing mip levels
    float lod0 = max(0.0, log2FootprintHalf - log2TexelSize_band0);
    float lod1 = max(0.0, log2FootprintHalf - log2TexelSize_band1);

    vec3 bestGuessNormal_band0 = SampleEstimatedNormalBand(
        DisplacementTexture_band0,
        NormalTexture_band0,
        worldXZ,
        invPatchSize_band0,
        patchSize_band0,
        displacementScale_band0,
        lod0
    );
   
    vec3 bestGuessNormal_band1 = SampleEstimatedNormalBand(
        DisplacementTexture_band1,
        NormalTexture_band1,
        worldXZ,
        invPatchSize_band1,
        patchSize_band1,
        displacementScale_band1,
        lod1
    );


    vec3 normal = normalize(bestGuessNormal_band0 + bestGuessNormal_band1);

    if (!gl_FrontFacing) {
        //normal *= -1.0;
    }

    if (u_mode == 1) {
        normal = bestGuessNormal_band0;
    }
    if (u_mode == 2) {
        normal = bestGuessNormal_band1;
    }


    // Converge to up normal over distance
    const float u_normalConvergeStartDist = 0.0;
    const float u_normalConvergeMaxDist = 250.0;
    const float u_normalConvergeMaxFactor = 0.9;
    const float u_normalConvergeExponent = 0.95;
    
    float t2 = clamp((viewDist - u_normalConvergeStartDist) / (u_normalConvergeMaxDist - u_normalConvergeStartDist), 0.0, 1.0);
    t2 = pow(t2, u_normalConvergeExponent) * u_normalConvergeMaxFactor;
    normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), t2));
    
    // Aggresively reduce normals
    float softenFactor = 0.5;
    normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), softenFactor));

    
    // High frequency normal ripples
    float rippleTiling = 0.325; 
    float rippleStrength = 0.025;
    vec2 rippleUV1 = worldXZ * rippleTiling + vec2(u_time * 0.15, u_time * 0.10);
    vec2 rippleUV2 = worldXZ * (rippleTiling * 1.5) + vec2(-u_time * 0.12, u_time * 0.08);
    //vec3 ripple1 = textureLod(DetailRippleNormal, rippleUV1, 2).xyz * 2.0 - 1.0;
    //vec3 ripple2 = textureLod(DetailRippleNormal, rippleUV2, 2).xyz * 2.0 - 1.0;
    vec3 ripple1 = texture(DetailRippleNormal, rippleUV1).xyz * 2.0 - 1.0;
    vec3 ripple2 = texture(DetailRippleNormal, rippleUV2).xyz * 2.0 - 1.0;
    vec3 microRipplesTangent = ripple1 + ripple2;
    vec3 rippleWorld = vec3(microRipplesTangent.x, 0.0, microRipplesTangent.y);
    normal = normal + (rippleWorld * rippleStrength);

    vec3 N = normalize(normal);
    vec3 V_view = normalize(viewPos - WorldPos);

    if (dot(N, V_view) < 0.0) {
        V_view = -V_view;
    }

    float roughness = 0.03;
    vec3 F0 = vec3(0.02); // should be 0.02 really

    // Precompute SSS height terms once (used by moon + flashlight)
    float h = WorldPos.y - u_oceanOriginY;
    float u_minHeight = u_oceanOriginY - 0.5;
    float u_maxHeight = u_oceanOriginY + 0.5;
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
    vec3 reflection_IBL = texture(cubeMap, R_rotated).rgb ;//* 0.75;
    reflection_IBL = pow(reflection_IBL, vec3(2.2));
    vec3 specular_IBL = reflection_IBL * kS_IBL;
    vec3 diffuse_IBL = moonColor * WATER_ALBEDO * 0.0125;

    surfaceLighting += Lo_direct;
    surfaceLighting += diffuse_IBL;
    surfaceLighting += specular_IBL;

    // SSS
    float sssStrength = 0.5;
    if (!gl_FrontFacing) sssStrength = 3; // Boost SSS strength when viewed underwater
    float minR = 0.45;
    float maxR = 0.60;
    float sssRadius = mix(0.45, 0.50, hNorm);
    float NdotL = max(dot(N, L), 0.0);
    float sssScalar = 0.2 * exp(-3.0 * abs(NdotL) / (sssRadius + 0.001));
    vec3 sss_albedo = WATER_ALBEDO;
    vec3 subColor = Saturate(sss_albedo, 2.0);
    vec3 sssColor = subColor * sssRadius * sssScalar * sssStrength;

    surfaceLighting += sssColor;


    // Flashlight
    //for (int i = 0; i < 2; i++ ) {
    //    float flashlightModifer = viewportDataArr[i].flashlightModifer;
    //
    //    if (flashlightModifer > 0.05) {
    //        vec3 flashlightColor = vec3(0.9, 0.95, 1.1);
    //
    //        vec3 spotLightPos = viewportDataArr[i].flashlightPosition.xyz;
    //        vec3 spotLightDir = normalize(viewportDataArr[i].flashlightDir.xyz);
    //        vec3 flashlightViewPos = viewportDataArr[i].inverseView[3].xyz;
    //        mat4 flashlightProjectionView = viewportDataArr[i].flashlightProjectionView;
    //
    //        vec3 L = normalize(spotLightPos - WorldPos);
    //        vec3 V = normalize(flashlightViewPos - WorldPos);
    //        float NoL = max(dot(N, L), 0.0);
    //
    //        float dist = length(spotLightPos - WorldPos);
    //        float lightRadius = 5.0;
    //        float strength = 3.0;
    //
    //        float innerAngle = cos(radians(5.0 * flashlightModifer));
    //        float outerAngle = cos(radians(25.0));
    //        float angleFactor = dot(L, -spotLightDir);
    //        float coneFalloff = smoothstep(outerAngle, innerAngle, angleFactor);
    //
    //        float distanceFalloff = smoothstep(lightRadius, 0.0, dist);
    //        float spotAttenuation = coneFalloff * distanceFalloff * distanceFalloff * strength;
    //
    //        vec3 cookie = ApplyCookie(flashlightProjectionView, WorldPos, spotLightPos, flashlightColor, 10, FlashlightCookieTexture);
    //
    //        vec3 spec_direct = microfacetSpecular(L, V, N, F0, roughness);
    //        vec3 Lo_direct = spec_direct * flashlightColor * NoL;
    //
    //        vec3 flashlightLighting = Lo_direct * spotAttenuation * cookie * flashlightModifer;
    //
    //        // Flashlight SSS
    //        vec3 subColor = Saturate(WATER_ALBEDO, 1.0);
    //        float NdotL_flash = max(dot(N, spotLightDir), 0.0);
    //        vec3 sss = 0.2 * exp(-3.0 * abs(NdotL_flash) / (radius + 0.01));
    //        vec3 sssColor = subColor * radius * sss * 1.5;
    //
    //        flashlightLighting += sssColor * flashlightModifer * coneFalloff;
    //
    //        surfaceLighting += flashlightLighting;
    //    }
    //}

    // Fog (reuse viewDist)
    {
        float u_fogStartDistance = 0.0;
        float u_fogEndDistance = 550.0;
        float u_fogExponent = 0.5;

        float fogRange = u_fogEndDistance - u_fogStartDistance;
        float normDist = (viewDist - u_fogStartDistance) / max(fogRange, 0.0001);
        normDist = clamp(normDist, 0.0, 1.0);

        float fogEffect = pow(normDist, u_fogExponent);
        float fogFactor = 1.0 - fogEffect;

        surfaceLighting = mix(u_fogColor * 0.1, surfaceLighting, fogFactor);
    }

    ColorOut = vec4(surfaceLighting, 1.0);

    if (gl_FrontFacing) {
        OceanMaskOut = 1u; // Top side
    }
    else {
        OceanMaskOut = 2u; // Bottom side
    }

    
    //if (gl_FrontFacing) {
    //    ColorOut.rgb = vec3(normal); 
    //}
    //ColorOut.rgb = vec3(specular_IBL); 
    //ColorOut.rgb = vec3(diffuse_IBL); 
    //ColorOut.rgb = v_debugColor;
    //ColorOut.rgb  = microRipples;
}
