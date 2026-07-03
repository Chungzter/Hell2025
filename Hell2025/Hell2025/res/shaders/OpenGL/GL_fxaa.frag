#version 410
layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 FragColor;

uniform sampler2D u_lightingTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_blueNoiseTexture;
uniform sampler2D u_blueNoise256Texture;
uniform float u_viewportWidth;
uniform float u_viewportHeight;
uniform int u_noiseSeed;

in vec2 TexCoords;
in vec2 offsetTL;
in vec2 offsetTR;
in vec2 offsetBL;
in vec2 offsetBR;
in vec2 texCoordOffset;

#define SCENE_NOISE_STRENGTH 0.02

vec3 Fxaa(sampler2D tex) {
    // fxaaSpanMax controls the maximum offset in texels that the algorithm will walk away from the center pixel when sampling along the edge.
    // Acceptable values are 4.0 or 8.0.
    const float fxaaSpanMax = 8.0;

    // fxaaReduceMin and fxaaReduceMul control how much the edge direction vector gets damped before it�s applied. 
    // These are the NVIDIA recommended tradeoffs, reducing the risk of false edges while still letting strong edges expand.
    const float fxaaReduceMin = 1.0 / 128.0;
    const float fxaaReduceMul = 1.0 / 8.0;

    const vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaTL = dot(luma, texture(tex, offsetTL).xyz);
    float lumaTR =  dot(luma, texture(tex, offsetTR).xyz);
    float lumaBL = dot(luma, texture(tex, offsetBL).xyz);
    float lumaBR = dot(luma, texture(tex, offsetBR).xyz);
    float lumaM  = dot(luma, texture(tex, TexCoords).xyz);
    
    vec2 dir = vec2(0.0);
    dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
    dir.y = ((lumaTL + lumaBL) - (lumaTR + lumaBR));
    
    float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (fxaaReduceMul * 0.25), fxaaReduceMin);
    float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(fxaaSpanMax, fxaaSpanMax), max(vec2(-fxaaSpanMax, -fxaaSpanMax), dir * inverseDirAdjustment)) * texCoordOffset;
    
    vec3 result1 = (1.0/2.0) * (texture(u_lightingTexture, TexCoords + (dir * vec2(1.0/3.0 - 0.5))).xyz + texture(u_lightingTexture, TexCoords + (dir * vec2(2.0/3.0 - 0.5))).xyz); 
    vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (texture(u_lightingTexture, TexCoords + (dir * vec2(0.0/3.0 - 0.5))).xyz + texture(u_lightingTexture, TexCoords + (dir * vec2(3.0/3.0 - 0.5))).xyz);
    
    float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
    float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
    float lumaResult2 = dot(luma, result2);
    
    if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
        return result1;
    else
        return result2;
}

// https://30fps.net/pages/post-processing/
// Make very bright colors to saturate to white like film does.
vec3 saturateToWhite(vec3 x) {
    // Compute a weighted channel-wise maximum, square it, add back.
    // Squaring makes 'white' blow up quickly when values go over 1.0
    float white = max(x.r * 0.3, max(x.g * 0.6, x.b * 0.1));
    x += vec3(white * white);
    return x;
}

vec3 colorGrading(vec3 color) {
    // "Color grading"
    color += 0.01 * vec3(1.0, 0.5, 0.1); // Lift blacks a tiny bit
    color *= vec3(1.0, 1.0, 1.1);        // Add blue gain
    color = pow(color, vec3(1.1));       // A subtle contrast boost
    
    // From https://iolite-engine.com/blog_posts/minimal_agx_implementation
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float sat = 0.9; // Adjust this
    color = luma + sat * (color - luma);
    return color;
}

vec3 addBlueNoise(vec3 color) {
    ivec2 j = ivec2(u_noiseSeed * 131, u_noiseSeed * 719);            // seed offset
    ivec2 q = (ivec2(gl_FragCoord.xy) + j) & 255;                     // sample coords
    float n = texelFetch(u_blueNoise256Texture, q, 0).r - 0.5;        // blue noise value in -0.5 to 0.5 range
    return clamp(color + (2.0 * SCENE_NOISE_STRENGTH) * n, 0.0, 1.0); // scaled result
}

void main() {
    float alpha = texture(u_lightingTexture, TexCoords).a;
    vec3 color = Fxaa(u_lightingTexture);
    
    // Dither to prevent banding on shadows and ssao
    const float ditherStrength = 1.0 / 255.0;
    int blueNoiseSize = textureSize(u_blueNoiseTexture, 0).x;
    ivec2 blueNoiseTexCoord = ivec2(gl_FragCoord.xy) % blueNoiseSize;
    float dither = texelFetch(u_blueNoiseTexture, blueNoiseTexCoord, 0).r;
    color += dither * ditherStrength;

    // Add blue noise
    vec3 normal = texture(u_normalTexture, TexCoords).rgb;
    float n2 = dot(normal, normal);
    if (n2 < 1e-6) {
        // Skip if skybox
    }
    else {
        color = addBlueNoise(color);
    }

    // Final color write
    FragColor = vec4(vec3(color), alpha);
}
