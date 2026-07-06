#version 460
#include "../../common/lighting.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 ColorOut;

in vec3 v_worldPos;
in vec2 v_uv;

uniform float u_particleAlphaFade;

layout (binding = 0) uniform samplerCube cubeMap;
layout (binding = 1) uniform sampler2D u_texture;

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
    float finalStrength = 0.2;
    //finalStrength = 1.0;

    // Flashlights
    for (int i = 0; i < 2; i++) {
        float modifier = viewportDataArr[i].flashlightModifer;

        if (modifier > 0.05) {
            vec4 flashlightDir = viewportDataArr[i].flashlightDir;
            vec4 flashlightPosition = viewportDataArr[i].flashlightPosition;

            vec3 spotLightPos = flashlightPosition.xyz;
            vec3 spotLightDir = normalize(flashlightDir.xyz);

            float spotLightRadius = 25.0;
            float spotLightStrength = 4.5;

            float innerAngle = cos(radians(5.0 * modifier));
            float outerAngle = cos(radians(20.5));

            // Attenuation
            float dist = length(spotLightPos - v_worldPos);
            float distanceFalloff = clamp(1.0 - (dist / spotLightRadius), 0.0, 1.0);
            distanceFalloff *= distanceFalloff;

            // Cone angle attenuation
            vec3 dirToFragment = normalize(v_worldPos - spotLightPos);
            float theta = dot(dirToFragment, spotLightDir);
            float epsilon = innerAngle - outerAngle;
            float coneFalloff = clamp((theta - outerAngle) / epsilon, 0.0, 1.0);

            // Accumulate flashlight contribution
            finalStrength += (distanceFalloff * coneFalloff * spotLightStrength * modifier);
        }
    }

    // Final color
    vec3 finalColor = spriteSheetColor.rgb * finalStrength;

    const vec3 UNDER_WATER_TINT = mix(vec3(0.4, 0.8, 0.6) * 1.75, vec3(0.01, 0.03, 0.04), 0.25);
    finalColor *= UNDER_WATER_TINT;

    a *= u_particleAlphaFade;
    a *= 0.0325;

    ColorOut = vec4(finalColor, a);
}
