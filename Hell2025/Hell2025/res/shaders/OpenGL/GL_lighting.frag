#version 450 core
out vec4 FragColor;

#include "../common/gl_fixed_bindings.glsl"

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

layout (binding = TEX_IDX_SHADOW_MAP_CSM)        uniform sampler2DArray shadowMap;

layout (binding = 4) uniform sampler2D diffuseTexture;
layout (binding = 5) uniform sampler2D noiseTexture;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;
uniform vec2 u_viewportSize;

uniform mat4 view;

layout(std430, binding = 0) readonly buffer lightProjectionViewsBuffer { mat4 lightProjectionViews[]; };

uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

vec3 gridSamplingDiskCSM[20] = vec3[](
    vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
    vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
    vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
    vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculationCSM(vec3 fragPosWorldSpace) {
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i) {
        if (depthValue < cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightProjectionViews[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0) {
        return 0.0;
    }

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fs_in.Normal);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 0.5f;
    float scaledBias = bias;
    if (layer == cascadeCount)
    {
        scaledBias *= 1.0 / (farPlane * biasModifier);
    }
    else
    {
        scaledBias *= 1.0 / (cascadePlaneDistances[layer] * biasModifier);
    }

    // Disk Sampling for PCF
    float shadow = 0.0;
    int samples = 20;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float diskRadius = 1.0; // Radius in texture space (adjust as needed)

    for (int i = 0; i < samples; ++i) {
        // Scale the disk sample by the texture size to get UV offsets
        vec2 offset = gridSamplingDiskCSM[i].xy * diskRadius * texelSize;
        float pcfDepth = texture(shadowMap, vec3(clamp(projCoords.xy + offset, 0.0, 1.0), layer)).r;
        shadow += (currentDepth - scaledBias) > pcfDepth ? 1.0 : 0.0;
    }

    shadow /= float(samples);

    return shadow;
}

void main() {
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    vec3 ambient = 0.3 * color;
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    float shadow = ShadowCalculationCSM(fs_in.FragPos);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

     ivec2 noiseTextureDimensions = textureSize(noiseTexture, 0);
    vec2 noiseTextureSize = vec2(noiseTextureDimensions);

    // Subpixel jittering using gl_FragCoord and noise texture size
    vec2 noiseUV = gl_FragCoord.xy / noiseTextureSize;
    vec2 jitter = texture(noiseTexture, noiseUV).xy - 0.5;

  //  vec2 jitter = texture(noiseTexture, gl_FragCoord.xy / u_viewportSize).xy; // Assuming you have a random texture and screen resolution
   // lighting = vec3(jitter, 0);

    FragColor = vec4(lighting, 1.0);
}
