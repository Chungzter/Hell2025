#version 460 core
layout(origin_upper_left) in vec4 gl_FragCoord;
layout(std430, binding = 6) readonly buffer HairMLAB {
    uvec4 nodes_v4[];
};

layout(binding = 0) uniform sampler2D u_SceneDepth;

uniform int u_renderTargetWidth;
uniform uint u_mlabFrameIndex;

in vec2 v_uv;
out vec4 FragColor;

vec4 UnpackMLABColor(uvec4 node) {
    vec2 rg = unpackHalf2x16(node.x);
    vec2 ba = unpackHalf2x16(node.y);
    return vec4(rg, ba);
}

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);
    uint pixelIdx = uint(p.y) * uint(u_renderTargetWidth) + uint(p.x);
    uint nodeBase = pixelIdx * 4u;

    float d_scene = texelFetch(u_SceneDepth, p, 0).r;

    vec4 acc = vec4(0.0);

    for (int i = 3; i >= 0; i--) {
        uvec4 node = nodes_v4[nodeBase + uint(i)];

        if (node.w != u_mlabFrameIndex || node.z == 0u) {
            continue;
        }

        float depth = uintBitsToFloat(node.z);

        if (depth < d_scene) {
            continue;
        }

        vec4 src = UnpackMLABColor(node);

        acc.rgb = src.rgb + acc.rgb * (1.0 - src.a);
        acc.a = src.a + acc.a * (1.0 - src.a);
    }

    FragColor = acc;
}
