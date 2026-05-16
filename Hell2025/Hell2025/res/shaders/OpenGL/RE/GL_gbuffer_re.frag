#version 460
#extension GL_ARB_bindless_texture : enable

#include "../../common/normal_encoding.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

in vec4 v_currPos;
in vec4 v_prevPos;
in vec2 v_uv;
in vec3 v_normal;
in vec3 v_tangent;

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];

    vec4 baseColor = texture(sampler2D(textureSamplers[renderItem.baseColorTextureIndex]), v_uv);
    vec3 normalMap = texture(sampler2D(textureSamplers[renderItem.normalMapTextureIndex]), v_uv).rgb;
    vec4 rma = texture(sampler2D(textureSamplers[renderItem.rmaTextureIndex]), v_uv).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[renderItem.emissiveTextureIndex]), v_uv).rgb;

    // Material
    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;

    // Normal mapping
    normalMap = normalMap * 2.0 - 1.0;
    vec3 n = normalize(v_normal);
    vec3 t = normalize(v_tangent);
    if (!gl_FrontFacing) n = -n;
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);
    vec3 normal = normalize(tbn * normalMap);

    // Velocity
    vec2 currNDC = v_currPos.xy / v_currPos.w;
    vec2 prevNDC = v_prevPos.xy / v_prevPos.w;
    vec2 velocity = currNDC - prevNDC;
    velocity *= 0.5;

    // Fix fireflies
    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5;
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    //float geometricRoughness = length(fwidth(normal));
    //roughness = clamp(roughness + geometricRoughness, 0.0, 1.0);

    //float filterRadius = 0.1;
    //float geometricRoughness = length(fwidth(normal));
    //roughness = max(roughness, geometricRoughness * filterRadius);

    //float normalMapVariation = length(fwidth(normalMap.rgb));
    //roughness = clamp(roughness + (normalMapVariation * 0.5), 0.0, 1.0);

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeNormal(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    // Velocity / Occlusion / Subsurface out
    VelocityXYOcclusionSubSurfaceOut.rg = velocity;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = 0.0; // Subsurface. Not quite sure what this is yet
}
