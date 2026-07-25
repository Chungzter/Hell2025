#version 460 core
#extension GL_ARB_bindless_texture : enable
layout(early_fragment_tests) in;

#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"

layout (location = 0) out vec4 FragOut;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS)            buffer textureSamplersBuffer   { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS)           buffer materialsBuffer         { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA)       buffer rendererDataBuffer      { RendererData rendererData;   };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA)       buffer viewportDataBuffer      { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS)              buffer lightsBuffer            { Light lights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_LIGHT_RANGES)  buffer glassLightRangesBuffer  { uvec2 glassLightRanges[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_LIGHT_INDICES) buffer glassLightIndicesBuffer { uint glassLightIndices[];};

layout(rgba16f, binding = 0) uniform image2D u_outputImage;
layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray FlashlighShadowMapTextureArray;

in vec2 v_uv;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec4 v_worldPos;
in vec3 v_viewPos;

in flat int v_materialIndex;
in flat uint v_instanceIndex;

uniform int u_viewportIndex;
uniform bool u_flipNormalMapY;

in vec3 v_tint;

void main() {

    ivec2 px = ivec2(gl_FragCoord.xy);
    vec4 lightingColor = imageLoad(u_outputImage, px);

    lightingColor *= vec4(v_tint, 1);

    imageStore(u_outputImage, px, lightingColor);

    //Material material = materials[v_materialIndex];
    //vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), v_uv);
    //vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), v_uv).rgb;
    //vec3 rma = texture(sampler2D(textureSamplers[material.rma]), v_uv).rgb;
    //
    //normalMap = mix(normalMap, vec3(0.5, 0.5, 1), 0.7);
    //
    //mat3 tbn = mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal));
    //normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    //normalMap = normalize(normalMap);
    //
    //if (u_flipNormalMapY) {
    //    normalMap.y *= -1;
    //}
    //
    //vec3 normal = normalize(tbn * (normalMap));
    //normal = clamp(normal, vec3(0), vec3(1));
    //
    //FragOut = vec4(normal, 0);;
}

void main2() {

    Material material = materials[v_materialIndex];
    vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), v_uv);
    vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), v_uv).rgb;
    vec3 rma = texture(sampler2D(textureSamplers[material.rma]), v_uv).rgb;

    normalMap = mix(normalMap, vec3(0.5, 0.5, 1), 0.7);

    mat3 tbn = mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal));
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1;
    }

    vec3 normal = normalize(tbn * (normalMap));

    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));
    float roughness = rma.r;
    float metallic = rma.g;

    //ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    //uint tileIndex = tile.y * rendererData.tileCountX + tile.x;
    //uint lightCount = tileData[tileIndex].lightCount;


    mat4 inverseView = viewportData[u_viewportIndex].inverseView;
    mat4 viewMatrix = viewportData[u_viewportIndex].view;
    vec3 viewPos = inverseView[3].xyz;


    vec3 directLighting = vec3(0);

    uvec2 range = glassLightRanges[v_instanceIndex];

    for (uint i = 0; i < range.y; i++) {
        uint lightIndex = glassLightIndices[range.x + i];
        Light light = lights[lightIndex];
        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);

        if (any(lessThan(v_worldPos.xyz, light.worldBoundsMin.xyz)) ||
            any(greaterThan(v_worldPos.xyz, light.worldBoundsMax.xyz))) {
            continue;
        }

        vec3 lightDelta = lightPos - v_worldPos.xyz;
        float distanceSquared = dot(lightDelta, lightDelta);

        if (distanceSquared > light.radius * light.radius) {
            continue;
        }

        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;


        vec3 lightVector = lightPos - v_worldPos.xyz;
        float safeDistanceSquared = max(distanceSquared, 0.0001);
        vec3 L = lightVector * inversesqrt(safeDistanceSquared);

        float ndotl = dot(normal, L);

        if (ndotl <= 0.0) {
            continue;
        }

        vec3 lightContribution = GetDirectLightingSpecularOnly(lightPos, lightColor, lightRadius, lightStrength, normal.xyz, v_worldPos.xyz, linearBaseColor.rgb, roughness, metallic, v_viewPos);

        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        }

        directLighting += lightContribution;
    }


    float fragDistance = distance(v_viewPos, v_worldPos.xyz);

    if (rendererData.flashlightIESTextureIndex >= 0) {
        sampler2D flashlightIES = sampler2D(textureSamplers[rendererData.flashlightIESTextureIndex]);
        for (int i = 0; i < 2; i++) {
            ViewportData flashlightViewportData = viewportData[i];
            directLighting += GetFlashlightContribution(i, uint(u_viewportIndex), flashlightViewportData.flashlightModifer, flashlightViewportData.flashlightProjectionView, flashlightViewportData.flashlightDir.xyz, flashlightViewportData.flashlightPosition.xyz, flashlightViewportData.inverseView[3].xyz, bool(flashlightViewportData.isInShop), rendererData, normal.xyz, v_worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, -1000.0, flashlightIES, FlashlighShadowMapTextureArray);
        }
    }


    vec3 finalColor = directLighting;
    FragOut.rgb = vec3(finalColor);
	FragOut.a = 1.0;



    //finalColor.rgb = vec3(1,0,0);

}
