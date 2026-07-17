#version 460 core
#include "../../common/normal_encoding.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 EmissiveOut;
layout (location = 3) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec3 v_normal;

uniform vec3 u_color;

void main() {
    float roughness = 0.8;
    float metallic = 0.1;
    float ao = 1.0;
    float thickness = 0.0;

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = u_color.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(v_normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    // Emissive
    EmissiveOut.rgb = vec3(0);
    EmissiveOut.a = thickness;

    // Velocity
    vec2 velocity = vec2(0, 0);
    VelocityXYOcclusionSubSurfaceOut = vec4(velocity, ao, 1.0);
}
