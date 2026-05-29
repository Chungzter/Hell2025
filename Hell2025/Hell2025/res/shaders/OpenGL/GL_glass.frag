#version 460 core

#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"

layout (location = 0) out vec4 FragOut;

layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

layout (binding = 7) uniform sampler2D FlashlightCookieTexture;
layout (binding = 8) uniform sampler2DArray FlashlighShadowMapTextureArray;

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData  rendererData;   };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData  viewportData[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer       { Light         lights[];       };

in vec2 TexCoord;
in vec3 v_normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec4 v_worldPos;
in vec3 ViewPos;
uniform int u_viewportIndex;
uniform bool u_flipNormalMapY;


void main() {

    vec4 baseColor = texture2D(baseColorTexture, TexCoord);
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    vec3 rma = texture2D(rmaTexture, TexCoord).rgb;

    normalMap = mix(normalMap, vec3(0.5, 0.5, 1), 0.7);

    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(v_normal));
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1;
    }

    vec3 normal = normalize(tbn * (normalMap));

    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));
    float roughness = rma.r;
    float metallic = rma.g;

    //ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    //uint tileIndex = tile.y * rendererData.tileCountX + tile.x;
    //uint lightCount = tileData[tileIndex].lightCount;


    mat4 inverseView = viewportData[u_viewportIndex].inverseView;
    mat4 viewMatrix = viewportData[u_viewportIndex].view;
    vec3 viewPos = inverseView[3].xyz;


    vec3 directLighting = vec3(0);

    //for (uint i = 0; i < lightCount; ++i) {
    //    uint lightIndex = tileData[tileIndex].lightIndices[i];
    //    Light light = lights[lightIndex];

    for (uint i = 0; i < 8; ++i) {
        Light light = lights[i];

        vec3 lightPos= vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        
        vec3 toLight = lightPos - v_worldPos.xyz;
        float dist = length(toLight);

        if (dist > lightRadius) continue;

        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float ndotl = dot(normal, L);
    
        if (ndotl <= 0.0) {
            continue;
        }

        vec3 lightContribution = GetDirectLightingSpecularOnly(lightPos, lightColor, lightRadius, lightStrength, normal.xyz, v_worldPos.xyz, linearBaseColor.rgb, roughness, metallic, ViewPos);
        
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        }

        directLighting += lightContribution;
    }


    mat4 flashlightProjectionView = viewportData[u_viewportIndex].flashlightProjectionView;
    vec4 flashlightDir = viewportData[u_viewportIndex].flashlightDir;
    vec4 flashlightPosition = viewportData[u_viewportIndex].flashlightPosition;
    float flashlightModifer = viewportData[u_viewportIndex].flashlightModifer;

    flashlightDir = viewportData[u_viewportIndex].cameraForward;
    flashlightPosition = viewportData[u_viewportIndex].viewPos;

    float fragDistance = distance(viewPos, v_worldPos.xyz);

    for (int i = 0; i < 2; i++) {
        float flashlightModifer = viewportData[i].flashlightModifer;
        if (flashlightModifer > 0.05) {
            mat4 flashlightProjectionView = viewportData[i].flashlightProjectionView;
            vec4 flashlightDir = viewportData[i].flashlightDir;
            vec4 flashlightPosition = viewportData[i].flashlightPosition;
            vec3 flashlightViewPos = viewportData[i].inverseView[3].xyz;
            vec3 playerForward = -normalize(viewportData[i].inverseView[2].xyz);
            int layerIndex = i;
		    vec3 spotLightPos = flashlightPosition.xyz;
            vec3 camightRight = normalize(viewportData[i].inverseView[0].xyz);
		    vec3 spotLightDir = flashlightDir.xyz;
            vec3 spotLightColor = vec3(0.9, 0.95, 1.1);

            float fresnelReflect = 0.9;
            float spotLightRadius = 20.0;
            float spotLightStregth = 4.5;

            // EXPERIMENTAL NEW COLORS
            vec3 defaultLightColor = vec3(1.0, 0.7799999713897705, 0.5289999842643738);
            spotLightColor = mix(defaultLightColor, spotLightColor, 0.95);
            spotLightRadius = 20.0;
            spotLightStregth = 5.0;

            if (v_worldPos.y < 10) {
                spotLightStregth = 25;
            }

            float innerAngle = cos(radians(5.0 * flashlightModifer));
            float outerAngle = cos(radians(25.0));
            mat4 lightProjectionView = flashlightProjectionView;
            vec3 spotLighting = GetSpotlightLighting(spotLightPos, spotLightDir, spotLightColor, spotLightRadius, spotLightStregth, innerAngle, outerAngle, normal.xyz, v_worldPos.xyz, linearBaseColor.rgb, roughness, metallic, flashlightViewPos, lightProjectionView);



            vec4 FragPosLightSpace = lightProjectionView * vec4(v_worldPos.xyz, 1.0);
            float shadow = SpotlightShadowCalculation(FragPosLightSpace, normal.xyz, spotLightDir, v_worldPos.xyz, spotLightPos, flashlightViewPos, FlashlighShadowMapTextureArray, layerIndex);

            vec3 cookie = ApplyCookie(lightProjectionView, v_worldPos.xyz, spotLightPos, spotLightColor, spotLightRadius, FlashlightCookieTexture);

            float cookieStartDistance = 1.0;
            float cookieEndDistance = 10.0;
            float cookieDistanceExponent = 2;
            float cookieMinValue = 0.5;
            float cookieMaxValue = 5.0;
            float cookieDistScale;
            if(fragDistance <= cookieStartDistance) {
                cookieDistScale = cookieMinValue;
            } else if(fragDistance >= cookieEndDistance) {
                cookieDistScale = cookieMaxValue;
            } else {
                float t = (fragDistance - cookieStartDistance) / (cookieEndDistance - cookieStartDistance);
                cookieDistScale = mix(cookieMinValue, cookieMaxValue, pow(t, cookieDistanceExponent));
            }
            spotLighting *= cookieDistScale;
            //spotLighting *= spotLightColor;

            spotLighting *= vec3(1 - shadow);
            spotLighting *= cookie;
            directLighting += vec3(spotLighting) * flashlightModifer;


            vec3 toLight = flashlightPosition.xyz - v_worldPos.xyz;
            float dist = length(toLight);
            vec3 lightDir = toLight / dist;
            vec3 viewDir = normalize(viewPos - v_worldPos.xyz);
            float att = smoothstep(spotLightRadius, 0.0, dist) * spotLightStregth;
            directLighting += vec3(roughness * roughness * 0.02 * att) * spotLightColor * cookie;
        }
    }


    vec3 finalColor = directLighting;
    FragOut.rgb = vec3(finalColor);
	FragOut.a = 1.0;



    //finalColor.rgb = vec3(1,0,0);

}
