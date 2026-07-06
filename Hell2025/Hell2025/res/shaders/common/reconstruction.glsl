vec3 WorldPosFromDepth_VK(vec2 viewportUV, float depth, mat4 inverseProjectionView) {
    vec2 clipXY = viewportUV * 2.0 - 1.0;
    vec4 clip = vec4(clipXY, depth, 1.0);
    vec4 worldH = inverseProjectionView * clip;
    return worldH.xyz / worldH.w;
}