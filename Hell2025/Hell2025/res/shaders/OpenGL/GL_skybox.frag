#version 460
#include "../common/types.glsl"

layout (location = 0) out vec4 FinalLightingOut;

layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;
in flat int ViewportIndex;

readonly restrict layout(std430, binding = 3) buffer viewportDataBuffer {
    ViewportData viewportData[];
};

vec3 GetWorldRay(vec2 fragCoordWindow, int viewportIndex) {
    vec2 viewportOrigin = vec2(viewportData[viewportIndex].xOffset, viewportData[viewportIndex].yOffset);
    vec2 viewportSize = vec2(viewportData[viewportIndex].width, viewportData[viewportIndex].height);
    vec2 fragCoord = fragCoordWindow - viewportOrigin;
    vec2 ndc = (fragCoord / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    mat4 inverseProjectionView = viewportData[viewportIndex].inverseProjectionView;

    vec4 nearH = inverseProjectionView * vec4(ndc, -1.0, 1.0);
    vec4 farH  = inverseProjectionView * vec4(ndc,  1.0, 1.0);
    vec3 nearW = nearH.xyz / nearH.w;
    vec3 farW  = farH.xyz  / farH.w;

    return normalize(farW - nearW);
}

void main() {
    vec3 skyColor = texture(cubeMap, TexCoords).rgb;
    vec3 skyLinear = pow(skyColor, vec3(2.6));

    vec3 rayDir = GetWorldRay(gl_FragCoord.xy, ViewportIndex);

    vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    vec3 downColor = vec3(0.4);

    float amount = 0.02;
    float colorCurve = 0.5;
    float fadeCurve = 0.9;

    float downwardness = clamp(-rayDir.y, 0.0, 1.0);
    float colorT = pow(downwardness, colorCurve);
    float fogT = pow(downwardness, fadeCurve);

    vec3 rayFogColor = mix(horizonColor, downColor, colorT) * amount;
    vec3 outColor = mix(skyLinear, rayFogColor, fogT);

    FinalLightingOut = vec4(outColor, 1.0);
}
