#version 450 core
#include "../../common/types.glsl"

uniform int u_mode = 0;

layout(binding = 0) uniform sampler2D DisplacementTexture_band0;
layout(binding = 1) uniform sampler2D NormalTexture_band0;
layout(binding = 2) uniform sampler2D DisplacementTexture_band1;
layout(binding = 3) uniform sampler2D NormalTexture_band1;

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform int u_viewportIndex;
uniform float u_oceanOriginY;

out vec3 v_worldPos;
out vec3 v_normal;
out vec3 v_debugColor;

const int LOD_LEVELS = 6;
const int GRID_SIZE = 128;
const float BASE_SPACING = 0.175;
const float LOD_SCALE = 2.0; // keeping this at 2.0 to ensure clean hole punching
const float FFT_RESOLUTION_BAND0 = 512.0;
const float FFT_RESOLUTION_BAND1 = 512.0;
const float PATCH_SIZE_BAND0 = 8.0;
const float PATCH_SIZE_BAND1 = 13.123;

void main() {
    vec3 viewPos = viewportDataArr[u_viewportIndex].viewPos.xyz;
    mat4 projectionView = viewportDataArr[u_viewportIndex].projectionViewReverseZ;

    int totalQuadsPerLOD = GRID_SIZE * GRID_SIZE;
    int quadID = gl_VertexID / 6;

    int lodLevel = quadID / totalQuadsPerLOD;
    int localQuadID = quadID % totalQuadsPerLOD;

    if (lodLevel >= LOD_LEVELS) {
        gl_Position = vec4(0.0);
        return;
    }

    int vertexID = gl_VertexID % 6;
    int quadX = localQuadID % GRID_SIZE;
    int quadY = localQuadID / GRID_SIZE;

    // hollowing out the center for the higher detail inner lods
    if (lodLevel > 0) {
        // leaving a slight overlap to hide snapping seams
        int margin = 2; 
        int holeStart = (GRID_SIZE / 4) + margin;
        int holeEnd = (GRID_SIZE * 3 / 4) - margin;

        if (quadX >= holeStart && quadX < holeEnd && quadY >= holeStart && quadY < holeEnd) {
            // outputting degenerate triangle to cull the quad
            gl_Position = vec4(0.0);
            return;
        }
    }

    // flipping mapped vertices to reverse the winding order
    int localX = (vertexID == 2 || vertexID == 3 || vertexID == 5) ? 1 : 0;
    int localY = (vertexID == 1 || vertexID == 4 || vertexID == 5) ? 1 : 0;

    float gridX = float(quadX + localX) - float(GRID_SIZE) * 0.5;
    float gridY = float(quadY + localY) - float(GRID_SIZE) * 0.5;

    float currentSpacing = BASE_SPACING * pow(LOD_SCALE, float(lodLevel));

    vec3 worldPos = vec3(gridX * currentSpacing, u_oceanOriginY, gridY * currentSpacing);

    // snapping camera to the current lod grid
    vec2 snappedCam = floor(viewPos.xz / currentSpacing) * currentSpacing;
    worldPos.x += snappedCam.x;
    worldPos.z += snappedCam.y;

    vec2 uv_band0 = fract(worldPos.xz / PATCH_SIZE_BAND0);
    vec2 uv_band1 = fract(worldPos.xz / PATCH_SIZE_BAND1);
    
    float displacementScale_band0 = PATCH_SIZE_BAND0 / FFT_RESOLUTION_BAND0;
    float displacementScale_band1 = PATCH_SIZE_BAND1 / FFT_RESOLUTION_BAND1;

    vec3 sample_band0 = textureLod(DisplacementTexture_band0, uv_band0, 0.0).xyz;
    vec3 sample_band1 = textureLod(DisplacementTexture_band1, uv_band1, 0.0).xyz;

    float deltaX_band0 = sample_band0.x * displacementScale_band0;
    float height_band0 = sample_band0.y * displacementScale_band0;
    float deltaZ_band0 = sample_band0.z * displacementScale_band0;
                                
    float deltaX_band1 = sample_band1.x * displacementScale_band1;
    float height_band1 = sample_band1.y * displacementScale_band1;
    float deltaZ_band1 = sample_band1.z * displacementScale_band1;
    
    float deltaX = deltaX_band0 + deltaX_band1;
    float height = height_band0 + height_band1;
    float deltaZ = deltaZ_band0 + deltaZ_band1;

    v_debugColor = mix(vec3(uv_band0, 0.0), vec3(uv_band1, 0.0), 0.5);

    if (u_mode == 1) {
        deltaX = deltaX_band0;
        height = height_band0;
        deltaZ = deltaZ_band0;
        v_debugColor = vec3(uv_band0, 0.0);
    }
    if (u_mode == 2) {
        deltaX = deltaX_band1;
        height = height_band1;
        deltaZ = deltaZ_band1;
        v_debugColor = vec3(uv_band1, 0.0);
    }
    
    // applying the displacement
    worldPos += vec3(deltaX, height, deltaZ);

    // dropping lower detail lods slightly after displacement
    // doing this so the higher detail mesh sits on top to resolve z fighting
    worldPos.y -= float(lodLevel) * 0.02;

    v_worldPos = worldPos;
    gl_Position = projectionView * vec4(worldPos, 1.0);
}