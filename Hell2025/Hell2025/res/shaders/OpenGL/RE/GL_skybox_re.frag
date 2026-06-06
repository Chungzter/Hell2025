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
    vec3 skyLinear = pow(skyColor, vec3(2.2));

    //vec3 finalColor = linearSkyColor;





    //vec3 skyColor = texture(cubeMap, TexCoords).rgb;
    //vec3 skyLinear = pow(skyColor, vec3(2.6));


    vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    vec3 downColor = vec3(0.4);

    float amount = 0.02;
    float colorCurve = 0.95;
    float fadeCurve = 0.69;

    float downwardness = clamp(-rayDir.y, 0.0, 1.0);
    float colorT = pow(downwardness, colorCurve);
    float fogT = pow(downwardness, fadeCurve);

    vec3 rayFogColor = mix(horizonColor, downColor, colorT) * amount;
    vec3 outColor = mix(skyLinear, rayFogColor, fogT);

    FinalLightingOut = vec4(outColor, 1.0);




    //float u_cutoffWorldY = viewPos.y;//0.1;
    //vec3 u_belowColorLinear = vec3(1,0,0);  
    //
    //// World ray under water fog
    //vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    //vec3 downColor = vec3(0.4);
    //
    //float amount = 0.0125;
    //float curve = 0.5; // higher = stays bright longer, lower = darkens faster
    //float downwardness = clamp(dot(rayDir, vec3(0.0, -1.0, 0.0)), 0.0, 1.0);
    //float t = pow(downwardness, curve); // 0 at horizon, 1 when looking down
    //vec3 finalRayFog = mix(horizonColor, downColor, t) * amount;
    //    
    //vec3 outColor = linearSkyColor;
    //float cutoffWorldY = viewPos.y;
    //
    //if (WorldPos.y < cutoffWorldY) {
    //    float fadeDistance = 50.0;   // world meters to reach full fog color
    //    float fadeExponent = 0.9;   // >1 slower near cutoff, <1 faster
    //
    //    float depthBelow = cutoffWorldY - WorldPos.y;          // 0 at cutoff, increases downward
    //    float f = clamp(depthBelow / fadeDistance, 0.0, 1.0);  // 0..1
    //    f = pow(f, fadeExponent);
    //
    //    // At cutoff: f=0 so black. Deeper: f->1 so finalRayFog.
    //    outColor = finalRayFog * f;
    //}
    
   // FinalLightingOut = vec4(finalColor, 1);
    //FinalLightingOut += vec4(1,0,0, 1);
}

