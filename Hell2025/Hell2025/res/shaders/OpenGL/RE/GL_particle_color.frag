#version 460
#extension GL_ARB_bindless_texture : enable
#include "../../common/lighting.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 ColorOut;

in vec3 v_worldPos;
in vec2 v_uv;
in float v_lifetime;

layout (binding = 0) uniform sampler2D u_texture;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 2) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

void main() {
    // Sample the flip book texture
    vec4 spriteSheetColor = texture(u_texture, v_uv);
    if (spriteSheetColor.a < 0.01) discard;

    spriteSheetColor.rgb = pow(spriteSheetColor.rgb, vec3(2.2));
    spriteSheetColor.rgb = pow(spriteSheetColor.rgb, vec3(2.2)); // Stack another POW for good measure
    float a = spriteSheetColor.a;

    // Base moonlight contribution
    vec3 finalLight = vec3(0.2);

    // Flashlights
    if (rendererData.flashlightIESTextureIndex >= 0) {
        sampler2D flashlightIES = sampler2D(textureSamplers[rendererData.flashlightIESTextureIndex]);
        for (int i = 0; i < 2; i++) {
            float modifier = viewportDataArr[i].flashlightModifer;
            if (modifier <= 0.05) continue;

            vec3 spotLightPos = viewportDataArr[i].flashlightPosition.xyz;
            vec3 spotLightDir = normalize(viewportDataArr[i].flashlightDir.xyz);
            float attenuation = GetFlashlightIESAttenuation(v_worldPos, spotLightPos, spotLightDir, rendererData, flashlightIES);
            finalLight += rendererData.flashlightColor.rgb * attenuation * modifier;
        }
    }

    // Final color
    vec3 finalColor = spriteSheetColor.rgb * finalLight;

    const vec3 UNDER_WATER_TINT = mix(vec3(0.4, 0.8, 0.6) * 1.75, vec3(0.01, 0.03, 0.04), 0.25);
    finalColor *= UNDER_WATER_TINT;

    // Dampen
    finalColor *= 0.5;


    a -= v_lifetime * 2;
    //a += 0.5;
    a = clamp(a, 0, 1);

    a *= 0.25;

    ColorOut = vec4(finalColor, a);
}
