#version 460 core

#ifndef ENABLE_BINDLESS
    #define ENABLE_BINDLESS 1
#endif

#if ENABLE_BINDLESS == 1
    #extension GL_ARB_bindless_texture : enable
    readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer {
	    uvec2 textureSamplers[];
    };
    in flat int BaseColorTextureIndex;
    in flat int NormalTextureIndex;
    in flat int RMATextureIndex;
    in flat int additionalTextureIndex0;
    in flat int WoundNormalTextureIndex;
    in flat int additionalTextureIndex2;

#else
    layout (binding = 0) uniform sampler2D baseColorTexture;
    layout (binding = 1) uniform sampler2D normalTexture;
    layout (binding = 2) uniform sampler2D rmaTexture;
    layout (binding = 3) uniform sampler2D emissiveTexture;
    layout (binding = 4) uniform sampler2D woundBaseColorTexture;
    layout (binding = 5) uniform sampler2D woundNormalTexture;
    layout (binding = 6) uniform sampler2D woundRmaTexture;
#endif

layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;

#include "../common/lighting.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/post_processing.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 EmissiveOut;
layout (location = 3) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec4 WorldPos;
in vec3 ViewPos;
in vec3 EmissiveColor;
in vec4 v_currPos;
in vec4 v_prevPos;

in flat int WoundMaskTextureIndex;
in flat int BlockBloodScreenSpaceDecalsFlag;
in flat int EmissiveTextureIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipNormalMapY;

void main() {
    vec3 emissiveColor = EmissiveColor;

#if ENABLE_BINDLESS == 1
    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoord).rgb;
    vec4 rmat = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[EmissiveTextureIndex]), TexCoord).rgb;
#else
    vec4 baseColor = texture(baseColorTexture, TexCoord);
    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    vec4 rmat = texture(rmaTexture, TexCoord).rgba;
    vec3 emissiveMapColor = texture(emissiveTexture, TexCoord).rgb;
#endif

    // Emissive
    if (EmissiveTextureIndex != -1) {
        emissiveColor *= emissiveMapColor;
    }
    EmissiveOut = vec4(emissiveColor, 0);

    if (u_alphaDiscard) {
        if (baseColor.a < 0.5) {
            discard;
        }
    }


    // Sensible defaults for wound texture
    vec4 woundBaseColor = vec4(0,0,0,0);
    vec3 woundNormalMap = vec3(0,0,0);
    vec3 woundRma = vec3(0,0,0);

    // If this mesh has a wound mask, then sample it
    float woundMask = 0;
    if (WoundMaskTextureIndex != -1) {
        #if ENABLE_BINDLESS == 1
            woundBaseColor = texture(sampler2D(textureSamplers[additionalTextureIndex0]), TexCoord);
            woundNormalMap = texture(sampler2D(textureSamplers[WoundNormalTextureIndex]), TexCoord).rgb;
            woundRma = texture(sampler2D(textureSamplers[additionalTextureIndex2]), TexCoord).rgb;
        #else
            woundBaseColor = texture(woundBaseColorTexture, TexCoord);
            woundNormalMap = texture(woundNormalTexture, TexCoord).rgb;
            woundRma = texture(woundRmaTexture, TexCoord).rgb;
        #endif
        woundMask  = texture(woundMaskTextureArray, vec3(TexCoord, WoundMaskTextureIndex)).r;

        // Hack to make the center of wounds black
        const float woundK = 0.1;
        const float woundDarkenStrength = 0.3;
        const float woundGamma = 0.1;
        woundMask = clamp(woundMask * 1.25, 0, 1);
        float t = clamp(pow(woundMask, woundGamma) * woundDarkenStrength, 0.0, 1.0);
        woundBaseColor.rgb = mix(woundBaseColor.rgb, vec3(0.0), t);
        woundRma.r = mix(woundRma.r, 0.0, t * 2);
        woundRma.b = mix(woundRma.b, 0.0, t * 2);
    }

    baseColor = mix(baseColor, woundBaseColor, woundMask);
    normalMap = mix(normalMap, woundNormalMap, woundMask);
    rmat.rgb = mix(rmat.rgb, woundRma, woundMask);

    vec3 n = normalize(Normal);
    vec3 t = normalize(Tangent - dot(Tangent, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);

    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1.0;
    };

    vec3 normal = normalize(tbn * (normalMap));

    if (!gl_FrontFacing) {
        normal = -normal;
    }

    float roughness = rmat.r;
    float metallic = rmat.g;
    float ao = rmat.b;


    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeNormal(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    //RMAOut.rgb = rmat.rgb;
    //RMAOut.a = BlockBloodScreenSpaceDecalsFlag;

    // Thickness
    float thickness = rmat.a;

    // Emissive
    EmissiveOut.a = thickness;

    // Velocity
    vec2 currNDC = v_currPos.xy / v_currPos.w;
    vec2 prevNDC = v_prevPos.xy / v_prevPos.w;
    vec2 velocity = currNDC - prevNDC;
    velocity *= 0.5;
    VelocityXYOcclusionSubSurfaceOut = vec4(velocity, ao, 1.0);


    //BaseColorOut.rgb = vec3(TexCoord, 0);


}
