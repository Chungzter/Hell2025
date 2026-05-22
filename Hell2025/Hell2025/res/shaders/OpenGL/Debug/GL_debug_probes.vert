 #version 450
#include "../../common/ddgi.glsl"
#include "../../common/types.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

layout(std430, binding = 6) readonly buffer ProbeColorBuffer  { ProbeColor probeColors[]; };
layout(std430, binding = 7) readonly buffer DDGIVolumeBuffer  { DDGIVolume volume; };
layout(std430, binding = 8) readonly buffer ProbeStatesBuffer { ProbeState probeStates[]; };

uniform mat4 u_projectionView;
flat out int v_probeIndex; 
flat out ivec3 v_voxelCoord;
out vec3 v_worldPos;
out vec3 v_normal;

void main() {
    v_probeIndex = gl_InstanceID;

    ivec3 probeCoords = DDGIGetProbeCoords(v_probeIndex, volume);
    vec3 probePos = DDGIGetProbeWorldPosition(probeCoords, volume, probeStates[v_probeIndex]);

    v_voxelCoord = probeCoords;

    float sphereScale = 0.0625;

    v_worldPos = probePos + a_position * sphereScale;
    v_normal = a_normal;

    gl_Position = u_projectionView * vec4(v_worldPos, 1.0);
}