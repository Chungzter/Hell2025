#version 460
#include "../../common/types.glsl"
#include "../../common/util.glsl"

layout (location = 0) out vec4 FinalLightingOut;

layout (binding = 0) uniform samplerCube u_skyboxCubeMap;

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

vec3 GetWorldRay(vec2 fragCoordWindow, uint viewportIndex) {
    vec2 viewportOrigin = vec2(viewportDataArr[viewportIndex].xOffset, viewportDataArr[viewportIndex].yOffset);
    vec2 viewportSize = vec2(viewportDataArr[viewportIndex].width, viewportDataArr[viewportIndex].height);
    vec2 fragCoord = fragCoordWindow - viewportOrigin;
    vec2 ndc = (fragCoord / viewportSize) * 2.0 - 1.0;
    mat4 inverseProjectionView = viewportDataArr[viewportIndex].inverseProjectionViewReverseZ;
    vec4 worldH = inverseProjectionView * vec4(ndc, 1.0, 1.0);
    vec3 worldPos = worldH.xyz / worldH.w;
    return normalize(worldPos - viewportDataArr[viewportIndex].viewPos.xyz);
}

mat3 GetSkyboxRotationMatrix() {
    float angle = radians(-90.0);
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c );
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uint viewportIndex = ComputeViewportIndexFromSplitscreenMode(px, resolution, rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataArr[viewportIndex];

    mat4 inverseView = viewportData.inverseView;
    vec3 viewPos = viewportData.viewPos.xyz;

    vec3 rayDir = GetWorldRay(gl_FragCoord.xy, viewportIndex);
    mat3 skyboxRotation = GetSkyboxRotationMatrix();
    vec3 skyboxSampleDir = normalize(skyboxRotation * rayDir);

    vec3 skyColor = texture(u_skyboxCubeMap, skyboxSampleDir).rgb;
    vec3 linearSkyColor = pow(skyColor, vec3(2.2));

    vec3 finalColor = linearSkyColor;

    FinalLightingOut = vec4(finalColor, 1);
}

