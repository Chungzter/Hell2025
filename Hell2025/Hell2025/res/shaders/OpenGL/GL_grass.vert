#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

uniform mat4 u_projectionView;
uniform mat4 u_prevProjectionView;
uniform mat4 u_rasterProjectionView;

out vec3 Normal;
out vec3 WorldPos;
out vec4 v_currPos;
out vec4 v_prevPos;

struct Vertex {
    float posX;
    float posY;
    float posZ;
    float normX;
    float normY;
    float normZ;
};

layout(std430, binding = SSBO_IDX_GRASS_POSITION_BLADE_POSITIONS) buffer bladePositions {
    vec4 BladePositions[];
};

layout(std430, binding = SSBO_IDX_GRASS_POSITION_INPUT_VERTICES) buffer inputVertexBuffer {
    Vertex InputVertexBuffer[];
};

layout(std430, binding = SSBO_IDX_GRASS_POSITION_INPUT_INDICES) buffer inputIndexBuffer {
    uint InputIndexBuffer[];
};

uint HashMix(vec2 v) {
    uint x = floatBitsToUint(v.x);
    uint y = floatBitsToUint(v.y);
    x ^= (x >> 17);
    y ^= (y << 13);
    x *= 374761393u;
    y *= 668265263u;
    x ^= (x >> 15);
    y ^= (y << 17);
    return x ^ y;
}

void main() {

    const uint indicesPerBlade = 24u;
    const uint hashMod = 360u;
    const uint verticesPerBlade = 12u;

    const uint basePosIndex = gl_VertexID / indicesPerBlade;
    const vec4 basePos = BladePositions[basePosIndex];

    const uint hashVal = HashMix(vec2(basePos.z, basePos.x));
    const uint baseVertex = (hashVal % hashMod) * verticesPerBlade;

    const uint index = baseVertex + (gl_VertexID % indicesPerBlade);
    const uint vertex = InputIndexBuffer[index];
    const Vertex v = InputVertexBuffer[vertex];

    WorldPos = vec3(v.posX, v.posY, v.posZ) + basePos.xyz;
    Normal = vec3(v.normX, v.normY, v.normZ);

    vec4 worldPos = vec4(WorldPos, 1.0);
    v_currPos = u_projectionView * worldPos;
    v_prevPos = u_prevProjectionView * worldPos;

	gl_Position = u_rasterProjectionView * worldPos;
}
