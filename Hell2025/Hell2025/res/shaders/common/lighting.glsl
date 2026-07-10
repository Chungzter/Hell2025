#include "../common/pbr_functions.glsl"
#include "../common/types.glsl"

#if defined(VULKAN)
float ApplyIESProfile(vec3 worldPos, Light light, texture2D iesTexture, sampler iesSampler) {
#else
float ApplyIESProfile(vec3 worldPos, Light light, sampler2D iesSampler) {
#endif
    vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
    vec3 forward = light.forward_iesMaxIntensity.rgb;
    vec3 right = light.right_iesExposure.rgb;
    vec3 up = light.up.rgb;
    float lightRadius = light.radius;
    float vScale = light.iesVScale;
    float vBias = light.iesVBias;
    float hScale = light.iesHScale;
    float hBias = light.iesHBias;
    float maxIntensity = light.forward_iesMaxIntensity.w;
    float exposure = light.right_iesExposure.w;
    const float globalDampener = 0.005;

    vec3 L = worldPos - lightPos;
    float dist = length(L);

    if (dist > lightRadius) return 0.0;

    vec3 dir = L / dist; // Normalized direction

    // Project into local space
    float dotF = dot(dir, forward);
    float dotR = dot(dir, right);
    float dotU = dot(dir, up);

    // U
    float theta = acos(clamp(dotF, -1.0, 1.0)) * 57.29578;
    float u = theta * vScale + vBias;

    // V
    float phi = atan(dotU, dotR) * 57.29578;
    float v = abs(phi) * hScale + hBias;

    // Compute mask
#if defined(VULKAN)
    float mask = texture(sampler2D(iesTexture, iesSampler), vec2(u, v)).r;
#else
    float mask = texture(iesSampler, vec2(u, v)).r;
#endif
    float atten = pow(clamp(1.0 - pow(dist / lightRadius, 4.0), 0.0, 1.0), 2.0) / (dist * dist + 1.0);
    return mask * maxIntensity * atten * exposure * globalDampener;
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0);

    // Hack to lesson nDotL blowouts from IES profile
    float wrap = 0.125; 
    ndl = clamp((ndl + wrap) / (1.0 + wrap), 0.0, 1.0);
    
    vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * att * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectLightingSpecularOnlyOLD(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0) * att;
    vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectLightingSpecularOnly(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);

    // falloff and light intensity
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0) * att;

    // calculate surface reflection only
    vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);

    // return the specular highlight for this light
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectionalLighting(vec3 lightDir, vec3 lightColor, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 viewDir = normalize(viewPos - WorldPos);
    float ndl = max(dot(Normal, lightDir), 0.0) * strength;
    vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}


vec3 GetSpotlightLighting(vec3 lightPos, vec3 lightDir, vec3 lightColor, float radius, float strength, float innerAngle, float outerAngle, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos, mat4 LightViewProj) {
    vec3 d = lightPos - WorldPos;
    float dist = length(d);
    vec3 toLight = d / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);

    // distance fall-off + strength
    float lightAttenuation = smoothstep(radius, 0.0, dist) * strength;

    // cone fall-off
    float spotFactor = smoothstep(outerAngle, innerAngle, dot(toLight, -lightDir));

    // extra smooth fade by distance
    float distanceFactor = clamp(1.0 - dist / radius, 0.0, 1.0);
    spotFactor *= distanceFactor * distanceFactor;

    // lambert
    float irradiance = max(dot(toLight, Normal), 0.0) * lightAttenuation * spotFactor;

    vec3 brdf = microfacetBRDF(toLight, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * irradiance * clamp(lightColor, 0.0, 1.0);
}

float SpotlightShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, vec3 fragWorldPos, vec3 lightPos, vec3 viewPos, sampler2DArray shadowMapArray, int layerIndex) {
    // Project and bias
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    float currentDepth = projCoords.z;

    // Fold slope bias and constant bias into one
    float dist = length(lightPos - fragWorldPos);
    float bias = 0.0001 + 0.028/(dist + 0.001);

    // Precompute texel size
    ivec2 size = textureSize(shadowMapArray, 0).xy;
    vec2 texelSize = 1.0/vec2(size);

    // PCF over 55 kernel
    float shadow = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float d = texture(shadowMapArray, vec3(projCoords.xy + vec2(x, y)*texelSize, layerIndex)).r;
            shadow += (currentDepth - bias > d) ? 1.0 : 0.0;
        }
    }

    // Average via multiply
    return shadow * (1.0 / 25.0);
}

float SpotlightShadowCalculationFast(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, vec3 fragWorldPos, vec3 lightPos, vec3 viewPos, sampler2DArray shadowMapArray, int layerIndex) {
    // Project and bias
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    float currentDepth = projCoords.z;

    // Fold slope bias and constant bias into one
    float dist = length(lightPos - fragWorldPos);
    float bias = 0.0001 + 0.028/(dist + 0.001);

    // Precompute texel size
    ivec2 size = textureSize(shadowMapArray, 0).xy;
    vec2 texelSize = 1.0/vec2(size);

    // Single sample (no PCF)
    float d = texture(shadowMapArray, vec3(projCoords.xy, layerIndex)).r;

    // Apply bias and check shadow
    float shadow = (currentDepth - bias > d) ? 1.0 : 0.0;

    // Return the final shadow factor
    return shadow;
}




#if defined(VULKAN)
vec3 ApplyCookie(mat4 LightViewProj, vec3 worldPos, vec3 lightPos, vec3 lightColor, float maxDistance, texture2D cookieTexture, sampler cookieSampler) {
#else
vec3 ApplyCookie(mat4 LightViewProj, vec3 worldPos, vec3 lightPos, vec3 lightColor, float maxDistance, sampler2D cookieTexture) {
#endif
   vec4 lightSpacePos = LightViewProj * vec4(worldPos, 1.0);
    vec2 cookieUV = lightSpacePos.xy / lightSpacePos.w * 0.5 + 0.5;

    // Centered scale
    float cookieScale = 1.5;
    vec2 scaledUV = (cookieUV - 0.5) * cookieScale + 0.5;

    // Edge fade based on scaledUV
    vec2 clampedUV = clamp(scaledUV, vec2(0.0), vec2(1.0));
    float fadeFactor = clamp(1.0 - length(scaledUV - clampedUV) * 15.0, 0.0, 1.0);

    float dist = length(worldPos - lightPos);
    float distanceFactor = clamp(1.0 - dist / maxDistance, 0.0, 1.0);
    distanceFactor *= distanceFactor;

#if defined(VULKAN)
    float cookieFactor = texture(sampler2D(cookieTexture, cookieSampler), clampedUV).r;
#else
    float cookieFactor = texture(cookieTexture, clampedUV).r;
#endif
    return lightColor * cookieFactor * fadeFactor * distanceFactor;
}

#if !defined(VULKAN)
vec3 GetFlashlightContribution(int flashlightIndex, uint viewportIndex, float flashlightModifer, mat4 flashlightProjectionView, vec3 flashlightDir, vec3 flashlightPosition, vec3 flashlightViewPos, bool flashlightIsInShop, vec3 flashlightColor, vec3 normal, vec3 worldPos, vec3 baseColor, float roughness, float metallic, float fragDistance, float oceanHeight, sampler2D flashlightCookieTexture, sampler2DArray flashlightShadowMapArrayTexture) {
    if (flashlightModifer <= 0.05) return vec3(0.0);

    int layerIndex = flashlightIndex;
    vec3 spotLightPos = flashlightPosition;
    vec3 spotLightDir = flashlightDir;
    vec3 spotLightColor = flashlightColor;
    float spotLightRadius = 25.0;
    float spotLightStregth = 4.5;

    if (worldPos.y < oceanHeight - 0.1) {
        spotLightStregth *= 2.0;
    }

    // Prevent flashlight being drawn on the back of your head when viewed by another player
    if (flashlightIndex != int(viewportIndex)) {
        spotLightPos += spotLightDir * 0.2;

        // and weaken it for other players
        spotLightColor *= 0.825;
    }

    float innerAngle = cos(radians(5.0 * flashlightModifer));
    float outerAngle = cos(radians(20.5));

    if (flashlightIsInShop) {
        spotLightRadius = 8;
        outerAngle = cos(radians(50.0));
    }

    mat4 lightProjectionView = flashlightProjectionView;
    vec3 spotLighting = GetSpotlightLighting(spotLightPos, spotLightDir, spotLightColor, spotLightRadius, spotLightStregth, innerAngle, outerAngle, normal, worldPos, baseColor, roughness, metallic, flashlightViewPos, lightProjectionView);

    vec4 FragPosLightSpace = lightProjectionView * vec4(worldPos, 1.0);
    float shadow = 0;

    // If this flashlight is in the shop AND this flashlight belongs to the current viewport
    if (flashlightIndex == int(viewportIndex) && flashlightIsInShop) {
        // do nothing
    }
    else {
        shadow = SpotlightShadowCalculation(FragPosLightSpace, normal, spotLightDir, worldPos, spotLightPos, flashlightViewPos, flashlightShadowMapArrayTexture, layerIndex);
    }

    vec3 cookie = ApplyCookie(lightProjectionView, worldPos, spotLightPos, spotLightColor, spotLightRadius, flashlightCookieTexture);

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

    spotLighting *= vec3(1 - shadow);

    if (!flashlightIsInShop) {
        spotLighting *= cookie;
    }

    return vec3(spotLighting) * flashlightModifer;
}
#endif



vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculationOLD(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArray shadowCubeMapArray) {
    vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;
    float shadow = 0.0;

    // Bias
    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005); 

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    for (int i = 0; i < samples; ++i) {
        // Sample with offset
        float closestDepth = texture(shadowCubeMapArray, vec4(lightToFrag + gridSamplingDisk[i] * diskRadius, lightIndex)).r;
        closestDepth *= far_plane;

        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples);
    return 1.0 - shadow;
}

float SamplePointShadowReceiverPlane(int lightIndex, vec3 lightToFrag, vec3 receiverNormal, float currentDepth, float lightRadius, float bias, vec3 sampleDir, float maxPlaneDepthDelta, samplerCubeArrayShadow shadowCubeMapArray) {
    vec3 sampleRay = normalize(sampleDir);

    float receiverDepth = currentDepth;
    float denominator = dot(receiverNormal, sampleRay);

    if (denominator < -0.03) {
        receiverDepth = dot(receiverNormal, lightToFrag) / denominator;
        receiverDepth = clamp(receiverDepth, currentDepth - maxPlaneDepthDelta, currentDepth + maxPlaneDepthDelta);
    }

    float compareDepth = clamp((receiverDepth - bias) / lightRadius, 0.0, 1.0);
    return texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
}


float ShadowCalculationSkin(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {
vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;

    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
    float compareDepth = clamp((currentDepth - bias) / far_plane, 0.0, 1.0);

    float visibility = 0.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        visibility += texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
    }

    return visibility / float(samples);
}

float ShadowCalculationNEW(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {

    vec3 lightToFrag = fragPos - lightPos;
   float currentDepth = length(lightToFrag);
   vec3 rayDir = lightToFrag / currentDepth;
   vec3 L = -rayDir;
  
   vec3 receiverNormal = normalize(Normal);
   if (dot(receiverNormal, L) < 0.0) {
       receiverNormal = -receiverNormal;
   }
  
   float cosTheta = clamp(dot(receiverNormal, L), 0.0, 1.0);
   float bias = max(0.05 * (1.0 - cosTheta), 0.005);
  
   float shadowMapSize = float(textureSize(shadowCubeMapArray, 0).x);
   float texelWorldSize = currentDepth * 2.0 / shadowMapSize;
  
   float softness = 2.25;
   float grazingScale = smoothstep(0.08, 0.45, cosTheta);
   float diskRadius = texelWorldSize * mix(0.75, softness, grazingScale);
   float maxPlaneDepthDelta = max(diskRadius * 8.0, 0.02);
  
   vec3 basisSeed = abs(rayDir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
   vec3 right = normalize(cross(basisSeed, rayDir));
   vec3 up = cross(rayDir, right);
  
   vec2 poissonDisk[8] = vec2[](
       vec2( 0.527,  0.085),
       vec2(-0.406,  0.331),
       vec2( 0.226, -0.543),
       vec2(-0.589, -0.205),
       vec2( 0.703, -0.391),
       vec2(-0.168,  0.743),
       vec2(-0.812,  0.125),
       vec2( 0.311,  0.379)
   );
  
   float visibility = 0.0;
  
   visibility += SamplePointShadowReceiverPlane(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, lightToFrag, maxPlaneDepthDelta, shadowCubeMapArray) * 2.0;
  
   for (int i = 0; i < 8; ++i) {
       vec2 disk = poissonDisk[i] * diskRadius;
       vec3 sampleDir = lightToFrag + right * disk.x + up * disk.y;
       visibility += SamplePointShadowReceiverPlane(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, sampleDir, maxPlaneDepthDelta, shadowCubeMapArray);
   }
  
   return visibility * 0.1;

  // vec3 lightToFrag = fragPos - lightPos;
  // vec3 L = normalize(-lightToFrag);
  // float currentDepth = length(lightToFrag);
  // float far_plane = lightRadius;
  // float shadow = 0.0;
  // 
  // // Bias
  // float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
  // float bias = max(0.05 * (1.0 - cosTheta), 0.005);
  // 
  // int samples = 20;
  // float viewDistance = length(viewPos - fragPos);
  // float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
  // 
  // for (int i = 0; i < samples; ++i) {
  //     vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
  //     float compareDepth = (currentDepth - bias) / far_plane;
  // 
  //     float visibility = texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
  // 
  //     shadow += 1.0 - visibility;
  // }
  // 
  // shadow /= float(samples);
  // return 1.0 - shadow;
}


float ShadowCalculationFastOLD(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArray shadowCubeMapArray) {
    vec3 lightDir = fragPos - lightPos;
    float currentDepth = length(lightDir);
    float far_plane = lightRadius;
    float shadow = 0.0;
    float bias = max(0.0125 * (1.0 - dot(Normal, normalize(lightDir))), 0.00125);  // Added normalize to lightDir
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    // Sample the cubemap array for shadows (single sample)
    float closestDepth = texture(shadowCubeMapArray, vec4(lightDir + gridSamplingDisk[0] * diskRadius, lightIndex)).r;
    closestDepth *= far_plane;  // Undo mapping [0;1]

    // Apply bias and check if the fragment is in shadow
    if (currentDepth - bias > closestDepth) {
        shadow = 1.0;
    }

    // Return the final shadow factor (1 means fully lit, 0 means fully in shadow)
    return 1.0 - shadow;
}


float ShadowCalculationMedium(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {
    vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;
    float shadow = 0.0;

    // Bias
    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 8;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        float compareDepth = (currentDepth - bias) / far_plane;

        float visibility = texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);

        shadow += 1.0 - visibility;
    }

    shadow /= float(samples);
    return 1.0 - shadow;
}










//vec3 GetDirectionalLighting(vec3 WorldPos, vec3 Normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos, vec3 lightDir, vec3 lightColor, float strength, float fresnelReflect) {
//	vec3 viewDir = normalize(viewPos - WorldPos);
//	float irradiance = max(dot(lightDir, Normal), 0.0) * strength;
//	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness);
//    return brdf * irradiance * clamp(lightColor, 0, 1);
//}














// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro

float D_GGX(float NoH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float b = (NoH2 * (alpha2 - 1.0) + 1.0);
    return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
  // Half vector
  vec3 H = normalize(V + L);

  // Dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);

  // Base reflectance (F0)
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect));
  f0 = mix(f0, baseColor, metallicness);

  // Fresnel term
  vec3 F = fresnelSchlick(VoH, f0);

  // Specular microfacet BRDF
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 specular = (D * G * F) / max(4.0 * NoV * NoL, 0.001);

  // Energy-conserving diffuse
  vec3 notSpecular = (1.0 - F) * (1.0 - metallicness);
  vec3 diffuse = notSpecular * baseColor / PI;

  return diffuse + specular;
}

vec3 microfacetSpecular(in vec3 L, in vec3 V, in vec3 N, in vec3 F0, in float roughness) {
    vec3 H = normalize(L + V);

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    vec3  F = fresnelSchlick(VoH, F0);

    return (D * G * F) / max(4.0 * NoV * NoL, 0.001);
}
