#version 460 core

#extension GL_ARB_bindless_texture : enable

#include "../../common/gl_fixed_bindings.glsl"

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow u_hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow u_lowResShadowMapArray;

layout (binding = 5) uniform sampler2D u_indirectDiffuseTexture;
layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;

layout(early_fragment_tests) in;

#include "../../common/hair.glsl"
#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"

readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = 4) buffer lightsBuffer { Light lights[]; };
readonly restrict layout(std430, binding = 5) buffer tileLightsBuffer { TileLights tileLights[]; };

layout(location = 0) out vec4 LightingOut;

centroid in vec2 v_texCoord;
centroid in vec3 v_normal;
centroid in vec3 v_tangent;
centroid in vec4 v_worldPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipv_normalMapY;
uniform float u_renderResolutionScale;
uniform int u_hairBlendMapTextureIndex;

const bool kHasBlendDecalMap = false; // cb12 g_sTexture.uiHasTex & 2
const uint kBlendDecalOp = 0u;       // cb12 g_sTexture.uiBlendDecalOp

bool transmissionEnabledForThisLight = false;
const bool kHasRoughnessMap = true;
const float kRoughnessGamma = 1.0;
const float kRoughnessWeight = 1.0;
const float kRawHairSpecularMask = 0.4313725490196078;
const float kDiffuseStrength = 0.5;
const float kTangentMapFlipGreen = 1; // 1.0;
const vec3 kBlackColorReflectionOffset = vec3(0.081, 0.106, 0.305);
const vec3 kWhiteColorReflectionOffset = vec3(0.0, 0.0, 0.0);

const vec3 kMaterialDiffuse = vec3(1.0, 1.0, 1.0);
const float kAOMapOccludeAllLighting = 0.0; // cb5[1].x

const vec3 kVertexGrayToColor = vec3(0.0, 0.0, 0.0);
const float kVertexColorStrength = 1.0;
const float kActiveChangeHairColor = 1.0;
const float kPaddingHasMips = 1.0;

const float kDecalWeight = 1.0;
const float kDecalGamma = 2.2;
const bool kHasDecalMap = true;

const bool kHasHairRootMap = true;
const bool kHasHairIdMap = false;

const bool kHasAOMap = true;
const float kAoWeight = 0.1;
const float kAoGamma = 1.0;

const float kBlendDecalWeight = 0.7000; // cb12[15].x, g_sTexture.fBlendDecalWeight
const float kBlendDecalGamma = 1.0000;  // cb12[17].w, g_sTexture.fBlendDecalGamma

const float kShadowOpacity = 0.0;       // cb13[41].x, g_fShadowOpacity
const float kShadowDarkenScale = 0.75;  // cb13[41].y, g_fShadowDarkenScale
const float kShadowSoftness = 1.0;      // cb13[41].z, g_fShadowSoftness
const uint kReceiveShadow = 1u;         // cb0[13].x, l_uReceiveShadow

const float scalar = 0.75;

const float kHairRoughnessMapStrength = 0.4410;
const float kHairSpecularMapStrength = 0.4310;
const float kSpecularStrength = 0.4600 * scalar;
const float kSecondarySpecularStrength = 2.7350 * scalar;
const float kTransmissionStrength = 0.1190;
const float kNoBackFaceTriangles = 0.8490;

float kLightIsIlluminate = 1.0;

float CC5ProcessAO(float rawAO) {
    float ao = exp2(log2(abs(rawAO)) * kAoGamma);
    return kAoWeight * (ao - 1.0) + 1.0;
}

float Pow5(float x) {
    float x2 = x * x;
    return x * x2 * x2;
}

float CC5Exp(float x) {
    return exp2(x * 1.442695);
}

float CC5FastAsinOLD(float x) {
    float absX = abs(x);
    float approx = 1.570796 - 0.156583 * absX;
    float root = sqrt(1.0 - absX);
    float angle = approx * root;
    float selectedAngle = x >= 0.0 ? angle : 3.141593 - angle;
    return 1.570796 - selectedAngle;
}

float CC5FastAsin(float x) {
    float absX = abs(x);
    float approx = 1.570796 - 0.156583 * absX;
    
    // clamp prevents negative floating point drift from generating NaN
    float root = sqrt(max(1.0 - absX, 0.0));
    
    float angle = approx * root;
    float selectedAngle = x >= 0.0 ? angle : 3.141593 - angle;
    return 1.570796 - selectedAngle;
}

vec3 BuildHairTangentFromFlow(vec3 tMesh, vec3 bMesh, vec2 flow) {
    float flowLen2 = dot(flow, flow);

    if (flowLen2 < 0.0001) {
        return normalize(tMesh);
    }

    vec2 flowDir = flow * inversesqrt(flowLen2);
    return normalize(tMesh * flowDir.x + bMesh * flowDir.y);
}

vec3 CC5HairSpecularPrimarySecondary_NoTransmission(
    vec3 T,
    vec3 V,
    vec3 L,
    vec3 hairBaseColor,
    float roughness,
    float hairSpecularMask,
    float specularStrength,
    float secondarySpecularStrength
) {
    float m = clamp(roughness, 0.0, 1.0);
    m = m * 0.98 + 0.02;

    float m2 = m * m;

    float dotVL = dot(V, L);
    float dotTL = dot(T, L);
    float dotTV = dot(T, V);

    float thetaV = CC5FastAsin(dotTV);
    float thetaL = CC5FastAsin(dotTL);

    float cosHalfThetaDiff = cos(abs(thetaV - thetaL) * 0.5);

    vec3 projectedL = L - dotTL * T;
    vec3 projectedV = V - dotTV * T;

    float projectedDot = dot(projectedL, projectedV);
    float projectedLenProduct = dot(projectedL, projectedL) * dot(projectedV, projectedV) + 0.0001;
    float cosPhi = projectedDot * inversesqrt(projectedLenProduct);

    float cosPhiHalf = sqrt(clamp(cosPhi * 0.5 + 0.5, 0.0, 1.0));

    float primaryWidth = cosPhiHalf * m2;
    float primarySigma = primaryWidth * 1.414214;
    float primaryNormalizer = primaryWidth * 3.544908;

    float sinThetaV = sqrt(1.0 - dotTV * dotTV);
    float azimuthShift = cosPhiHalf * 0.997551 * sinThetaV + dotTV * -0.069943;

    float primaryX = dotTL + dotTV + azimuthShift * 0.139886;
    float primaryD = CC5Exp((primaryX * primaryX * -0.5) / (primarySigma * primarySigma)) / primaryNormalizer;

    float primaryFresnelBase = 1.0 - sqrt(clamp(dotVL * 0.5 + 0.5, 0.0, 1.0));
    float primaryFresnel = Pow5(primaryFresnelBase) * 0.953479 + 0.046521;

    float primaryAzimuthFactor = cosPhiHalf * 0.25;
    float primaryFacingGate = 1.0 - clamp(-dotVL, 0.0, 1.0);

    float primarySpecular = primaryD;
    primarySpecular *= specularStrength;
    primarySpecular *= primaryAzimuthFactor;
    primarySpecular *= primaryFresnel;
    primarySpecular *= hairSpecularMask + hairSpecularMask;
    primarySpecular *= primaryFacingGate;
    
    
    //float test = CC5Exp((primaryX * primaryX * -0.5) / (primarySigma * primarySigma));// / primaryNormalizer;
    //return vec3(test);

    float secondarySigma = m2 + m2;
    float secondaryNormalizer = m2 * 5.013257;
    float secondaryX = dotTL + dotTV - 0.140000;

    float secondaryD = CC5Exp((secondaryX * secondaryX * -0.5) / (secondarySigma * secondarySigma)) / secondaryNormalizer;

    float secondaryFresnelBase = 1.0 - cosHalfThetaDiff * 0.5;
    float secondaryFresnel = Pow5(secondaryFresnelBase) * 0.953479 + 0.046521;
    secondaryFresnel *= (1.0 - secondaryFresnel) * (1.0 - secondaryFresnel);

    float secondaryTintPower = 0.8 / cosHalfThetaDiff;
    vec3 secondaryTint = exp2(log2(abs(hairBaseColor)) * secondaryTintPower);

    float secondaryPhiBoost = CC5Exp(cosPhi * 17.0 - 16.780001);

    float secondarySpecular = secondaryD;
    secondarySpecular *= secondarySpecularStrength;
    secondarySpecular *= secondaryPhiBoost;
    secondarySpecular *= secondaryFresnel;

    vec3 specular = vec3(primarySpecular);
    specular += secondarySpecular * secondaryTint;

    return specular;
}

float CC5HairDiffuseScalar(vec3 T, vec3 V, vec3 L, vec3 N, float diffuseStrength, float noBackFaceTriangles) {
    float dotTL = dot(T, L);
    float dotTV = dot(T, V);

    vec3 projectedV = V - dotTV * T;
    float projectedVLengthSquared = dot(projectedV, projectedV);
    vec3 projectedVNormalized = projectedV * inversesqrt(projectedVLengthSquared);

    float hairScatter = dot(projectedVNormalized, L);
    hairScatter = clamp((hairScatter + 1.0) * 0.25, 0.0, 1.0);

    float tangentLightFactor = 1.0 - abs(dotTL);
    float hairDiffuse = hairScatter + (tangentLightFactor - hairScatter) * 0.33;

    float standardDiffuse = clamp(dot(N, L), 0.0, 1.0) * diffuseStrength;
    float hairDiffuseScaled = hairDiffuse * diffuseStrength;

    return standardDiffuse + noBackFaceTriangles * (hairDiffuseScaled - standardDiffuse);
}

vec3 CC5DecodeColor(vec3 color, float gamma) {
    return exp2(log2(abs(color)) * gamma);
}


vec3 CC5ComputeHairBaseColorFromCurrentInputs(vec3 baseColorRGB, vec3 decodedBlendMultiply, float vertexColorX) {
    vec3 hairBaseColor;

    bool useHairAutoBakedBaseColor = (kHasHairRootMap || kHasHairIdMap) && (kActiveChangeHairColor > 0.0);

    if (useHairAutoBakedBaseColor) {
        hairBaseColor = CC5DecodeColor(baseColorRGB, 2.2);
    } else {
        hairBaseColor = kHasDecalMap ? CC5DecodeColor(baseColorRGB, kDecalGamma) : vec3(1.0);
    }

    float vertexGrayBlend = 1.0 - (kVertexColorStrength * (vertexColorX - 1.0) + 1.0);
    hairBaseColor = clamp(vertexGrayBlend * (kVertexGrayToColor - hairBaseColor) + hairBaseColor, 0.0, 1.0);

    vec3 decalWeightedColor = kDecalWeight * (hairBaseColor - vec3(1.0)) + vec3(1.0);
    hairBaseColor = kHasDecalMap ? decalWeightedColor : hairBaseColor;

    if (kHasBlendDecalMap) {
        if (kBlendDecalOp == 0u) {
            vec3 blendMultiplyFactor = kBlendDecalWeight * (decodedBlendMultiply - vec3(1.0)) + vec3(1.0);
            hairBaseColor *= blendMultiplyFactor;
        } else if (kBlendDecalOp == 1u) {
            hairBaseColor = kBlendDecalWeight * decodedBlendMultiply + hairBaseColor;
        } else {
            vec3 blend = kBlendDecalWeight * (decodedBlendMultiply - vec3(0.5)) + vec3(0.5);
            vec3 multiplySide = hairBaseColor * blend * 2.0;
            vec3 screenSide = 1.0 - ((1.0 - hairBaseColor) * 2.0) * (1.0 - blend);
            hairBaseColor = mix(screenSide, multiplySide, lessThan(hairBaseColor, vec3(0.5)));
        }
    }

    hairBaseColor *= kMaterialDiffuse;

    return hairBaseColor;
}


vec3 RecoverRawTextureData(vec3 hardwareLinear) {
    bvec3 cutoff = lessThan(hardwareLinear, vec3(0.0031308));
    vec3 lower = hardwareLinear * 12.92;
    vec3 higher = 1.055 * pow(hardwareLinear, vec3(1.0 / 2.4)) - vec3(0.055);
    return mix(higher, lower, cutoff);
}




float FastAcosApprox(float x) {
    float ax = abs(x);
    float a = ax * -0.156583 + 1.570796;
    float b = sqrt(max(1.0 - ax, 0.0));
    float positiveResult = b * a;
    float negativeResult = 3.141593 - positiveResult;
    return x >= 0.0 ? positiveResult : negativeResult;
}




void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];

    sampler2D baseColorSampler = sampler2D(textureSamplers[renderItem.baseColorTextureIndex]);
    sampler2D rmaSampler = sampler2D(textureSamplers[renderItem.rmaTextureIndex]);
    sampler2D additionalSampler0 = sampler2D(textureSamplers[renderItem.additionalTextureIndex0]);
    sampler2D additionalSampler1 = sampler2D(textureSamplers[renderItem.additionalTextureIndex1]);
    sampler2D additionalSampler2 = sampler2D(textureSamplers[renderItem.additionalTextureIndex2]);

    sampler2D blendMapSampler    = sampler2D(textureSamplers[u_hairBlendMapTextureIndex]);
    vec3 blendMultiply  = texture(blendMapSampler, v_texCoord).rgb;
    vec3 decodedBlendMultiply = CC5DecodeColor(blendMultiply.rgb, kBlendDecalGamma);

    vec4 baseColorOG = texture(baseColorSampler, v_texCoord);
    vec4 rma = texture(rmaSampler, v_texCoord);
    vec4 additionalColor0 = texture(additionalSampler0, v_texCoord);
    vec4 additionalColor1 = texture(additionalSampler1, v_texCoord);
    vec4 additionalColor2 = texture(additionalSampler2, v_texCoord);

    vec2 baseTextureSizePixels = vec2(textureSize(baseColorSampler, 0));
    float hairMipLevelRaw = ComputeHairMipLevel(v_texCoord, baseTextureSizePixels);
   

    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;

    vec3 flowSample = additionalColor0.rgb;
    
    //vec3 flowSample = vec3(1.0, 0.5, 0.5);

    float hairId = additionalColor1.r;
    float rootFactor = additionalColor2.r;

    float metallicOLD = rma.g;
    float rawAO = rma.b;

    float aoFactor = kHasAOMap ? CC5ProcessAO(rawAO) : 1.0;


    float vertexColorX = 0.8235;// v_vertexColorX; // this is an average of the values found in the cc5 geometry

    bool useHairAutoBakedBaseColor = (kHasHairRootMap || kHasHairIdMap) && (kActiveChangeHairColor > 0.0);
    bool hasDecalMap = kHasDecalMap;
    bool paddingHasMips = kPaddingHasMips > 0.0;

    
    float autoBakedSpecularMask = baseColorOG.a;
    float hairSpecularMask = useHairAutoBakedBaseColor
        ? autoBakedSpecularMask
        : 1.0 * kHairSpecularMapStrength;

        vec3 hairBaseColor = CC5ComputeHairBaseColorFromCurrentInputs(baseColorOG.rgb, decodedBlendMultiply, vertexColorX);

    if (kHasAOMap && kAOMapOccludeAllLighting > 0.0) {
        hairBaseColor *= aoFactor;
    }

    vec3 cc5View = normalize(viewPos - v_worldPos.xyz);



    vec3 normalWS = normalize(v_normal);
    if (!gl_FrontFacing) {
        normalWS = -normalWS;
    }
    
    vec3 tangentWS = normalize(v_tangent);
    //tangentWS = normalize(tangentWS - dot(tangentWS, normalWS) * normalWS);
    
    vec3 binormalWS = normalize(cross(normalWS, tangentWS));
    //vec3 binormalWS = normalize(cross(tangentWS, normalWS));




    //vec3 dp1 = dFdx(v_worldPos.xyz);
    //vec3 dp2 = dFdy(v_worldPos.xyz);
    //vec2 duv1 = dFdx(v_texCoord);
    //vec2 duv2 = dFdy(v_texCoord);
    //
    //float det = duv1.x * duv2.y - duv2.x * duv1.y;
    //tangentWS = vec3(0.0);
    //binormalWS = vec3(0.0);
    //
    //// Prevent division by zero on degenerate triangles
    //if (abs(det) > 0.00001) {
    //    float invDet = 1.0 / det;
    //    tangentWS = normalize((dp1 * duv2.y - dp2 * duv1.y) * invDet);
    //    binormalWS = normalize((dp2 * duv1.x - dp1 * duv2.x) * invDet);
    //} else {
    //    // Fallback if derivatives fail
    //    tangentWS = normalize(v_tangent);
    //    binormalWS = normalize(cross(normalWS, tangentWS));
    //}
    //
    //// 3. Gram-Schmidt to ensure a perfect 90-degree orthogonal basis
    //tangentWS = normalize(tangentWS - dot(tangentWS, normalWS) * normalWS);
    //binormalWS = normalize(binormalWS - dot(binormalWS, normalWS) * normalWS - dot(binormalWS, tangentWS) * tangentWS);
















    vec3 flow = flowSample * 2.0 - 1.0;

    vec3 baseHairTangentTS = vec3(
        flow.x,
        flow.y * kTangentMapFlipGreen,
        0.0 
    );

    vec3 shiftedHairTangentTS = normalize(baseHairTangentTS);
    
    if (kHasHairIdMap) {
        vec3 reflectionOffsetTS = mix(kBlackColorReflectionOffset, kWhiteColorReflectionOffset, hairId);
        shiftedHairTangentTS += reflectionOffsetTS;
    }

    vec3 cc5Tangent = normalize(
        tangentWS * shiftedHairTangentTS.x +
        binormalWS * shiftedHairTangentTS.y +
        normalWS * shiftedHairTangentTS.z
    );

    vec3 n = normalWS;

    float roughnessOLD = 0.5;

    if (kHasRoughnessMap) {
        float rawRoughness = texture(rmaSampler, v_texCoord).r;
        roughnessOLD = exp2(log2(abs(rawRoughness)) * kRoughnessGamma);
        roughnessOLD *= kRoughnessWeight;
    }
    
    roughnessOLD *= kHairRoughnessMapStrength;

    shiftedHairTangentTS = normalize(shiftedHairTangentTS);

    vec3 cc5Normal = normalWS;

    










    
    


   // bool verfiyTangents = true;
   // if (verfiyTangents) {
   //     vec3 dp1 = dFdx(v_worldPos.xyz);
   //     vec3 dp2 = dFdy(v_worldPos.xyz);
   //     vec2 duv1 = dFdx(v_texCoord);
   //     vec2 duv2 = dFdy(v_texCoord);
   // 
   //     float det = duv1.x * duv2.y - duv2.x * duv1.y;
   //     meshTangent = vec3(0.0);
   //     meshBitangent = vec3(0.0);
   // 
   //     if (abs(det) > 0.00001) {
   //         float invDet = 1.0 / det;
   //         meshTangent = (dp1 * duv2.y - dp2 * duv1.y) * invDet;
   //         meshBitangent = (dp2 * duv1.x - dp1 * duv2.x) * invDet;//

   //         meshTangent = normalize(meshTangent - dot(meshTangent, meshNormalUnflipped) * meshNormalUnflipped);
   //         meshBitangent = normalize(meshBitangent - dot(meshBitangent, meshNormalUnflipped) * meshNormalUnflipped - dot(meshBitangent, meshTangent) * meshTangent);
   //     } 
   //     else {
   //         meshTangent = normalize(v_tangent);
   //         meshTangent = normalize(meshTangent - dot(meshTangent, meshNormalUnflipped) * meshNormalUnflipped);
   //         meshBitangent = normalize(cross(meshNormalUnflipped, meshTangent));
   //     }
   // }






















    bool overrideLight = true;
    bool overrideTangent = true;

    sampler2D hairAutoBakedBaseColorSampler = sampler2D(textureSamplers[renderItem.baseColorTextureIndex]);
    sampler2D flowSampler = sampler2D(textureSamplers[renderItem.additionalTextureIndex0]);
    sampler2D hairIDSampler = sampler2D(textureSamplers[renderItem.additionalTextureIndex1]);
    sampler2D rootSampler = sampler2D(textureSamplers[renderItem.additionalTextureIndex2]);
    
    vec3 meshTangent = normalize(v_tangent);

    vec3 meshNormalUnflipped = normalize(v_normal);
    vec3 meshNormal = gl_FrontFacing ? meshNormalUnflipped : -meshNormalUnflipped;

    vec3 meshBitangent = normalize(cross(meshNormalUnflipped, meshTangent));

    vec3 flowMap = texture(flowSampler, v_texCoord).xyz;
    flowMap = flowMap * 2.0 - 1.0;

    vec3 tangentSpaceShift;
    tangentSpaceShift.x = flowMap.x;
    tangentSpaceShift.y = flowMap.y * kTangentMapFlipGreen;
    tangentSpaceShift.z = flowMap.z * 0.03;

    float hairID = texture(hairIDSampler, v_texCoord).r;

    vec3 blackOffset = vec3(-0.206, -0.687, -0.338);
    vec3 whiteOffset = vec3(-0.148, 0.0, 0.370);
    vec3 idOffset = mix(blackOffset, whiteOffset, hairID);

    tangentSpaceShift = normalize(tangentSpaceShift + idOffset);

    vec3 finalTangent = normalize(
        tangentSpaceShift.x * meshTangent +
        tangentSpaceShift.y * meshBitangent +
        tangentSpaceShift.z * meshNormal
    );

    vec3 bumpNormal = vec3(0.0, 0.0, 1.0); // There is no normal map for this hairBaseColor

    vec3 finalNormal = normalize(
        bumpNormal.x * meshTangent +
        bumpNormal.y * meshBitangent +
        bumpNormal.z * meshNormal
    );






    vec3 v_color = vec3(0.8235, 0, 0); // This is an average of the values found in the cc5 geometry

    vec3 baseColor = texture(hairAutoBakedBaseColorSampler, v_texCoord).rgb;
    baseColor = pow(abs(baseColor), vec3(2.2));
    float vertexGrayBlend = kVertexColorStrength * (1.0 - v_color.r);
    baseColor = clamp(mix(baseColor, kVertexGrayToColor, vertexGrayBlend), 0.0, 1.0);
    //baseColor = mix(vec3(1.0), baseColor, kDecalWeight);
    baseColor *= kMaterialDiffuse;

    float ao = 1.0;

    if (kHasAOMap) {
        ao = texture(rmaSampler, v_texCoord).b; // In my engine, AO is stored in the b channel of the material rma texture
        ao = pow(abs(ao), kAoGamma);
        ao = 1.0 + kAoWeight * (ao - 1.0);
    }

    if (kAOMapOccludeAllLighting > 0.0 && kHasAOMap) {
        baseColor *= ao;
    }

    float roughness = 0.5;

    if (kHasRoughnessMap) {
        roughness = texture(rmaSampler, v_texCoord).r;  // In my engine, roughness is stored in the r channel of the material rma texture
        roughness = pow(abs(roughness), kRoughnessGamma);
        roughness *= kRoughnessWeight;
    }

    roughness *= kHairRoughnessMapStrength;

    float metallic = 0.0; // Always assume 0 metallic for hair





    float shadow = 1.0;
    shadow = shadow * (1.0 - kShadowOpacity) + kShadowOpacity;
    shadow = 1.0 + float(kReceiveShadow) * (shadow - 1.0);

    vec3 viewDirection = normalize(viewPos - v_worldPos.xyz);

    vec3 diffuseColor = baseColor * (1.0 - metallic);
    vec3 specularColor = mix(vec3(0.03), baseColor, metallic);

    //float lightVisibility = shadow * kLightIsIlluminate;




    bool enableSecondarySpecular = true;

    //vec3 kLightDirection = vec3(0, -1, 0);
    //vec3 lightDirection = normalize(-kLightDirection);

    int i = 2;
    Light light = lights[i];
    vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
    vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);
    float lightStrength = light.strength;
    float lightRadius = light.radius;

    vec3 toLight = lightPos - v_worldPos.xyz;
    float lightDistance = length(toLight);
    vec3 lightDirection = toLight / max(lightDistance, 0.00001);

    float lightAttenuation = smoothstep(lightRadius, 0.0, lightDistance) * lightStrength;

    float lightShadow = 1.0;
    if (light.hiResShadowMapIndex != -1) {
        lightShadow = ShadowCalculationNEW(light.hiResShadowMapIndex, lightPos, lightRadius, v_worldPos.xyz, viewPos, n, u_hiResShadowMapArray);
    }
    else if (light.lowResShadowMapIndex != -1) {
        lightShadow = ShadowCalculationNEW(light.lowResShadowMapIndex, lightPos, lightRadius, v_worldPos.xyz, viewPos, n, u_lowResShadowMapArray);
    }

    vec3 lightColor = clamp(lightCol, 0.0, 1.0);
    float lightVisibility = lightAttenuation * lightShadow;

    if (light.iesTextureIndex != 0) {
        sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
        float candelas = ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        lightVisibility *= candelas;
    }








    float hairRoughness = clamp(roughness, 0.0, 1.0);
    hairRoughness = hairRoughness * 0.98 + 0.02;

    float VdotL = dot(viewDirection, lightDirection);
    float TdotL = dot(finalTangent, lightDirection);
    float TdotV = dot(finalTangent, viewDirection);

    float tangentViewAngle = FastAcosApprox(TdotV);
    float tangentLightAngle = FastAcosApprox(TdotL);

    float tangentViewSinAngle = 1.570796 - tangentViewAngle;
    float tangentLightSinAngle = 1.570796 - tangentLightAngle;

    float cosHalfTangentAngleDifference = cos(abs(tangentViewSinAngle - tangentLightSinAngle) * 0.5);

    vec3 lightPerpendicularToTangent = lightDirection - finalTangent * TdotL;
    vec3 viewPerpendicularToTangent = viewDirection - finalTangent * TdotV;

    float perpendicularDot = dot(lightPerpendicularToTangent, viewPerpendicularToTangent);
    float lightPerpendicularLengthSq = dot(lightPerpendicularToTangent, lightPerpendicularToTangent);
    float viewPerpendicularLengthSq = dot(viewPerpendicularToTangent, viewPerpendicularToTangent);

    float normalizedPerpendicularDot = perpendicularDot * inversesqrt(lightPerpendicularLengthSq * viewPerpendicularLengthSq + 0.0001);

    float halfAzimuth = normalizedPerpendicularDot * 0.5 + 0.5;
    halfAzimuth = clamp(halfAzimuth, 0.0, 1.0);

    float cosHalfAzimuth = sqrt(halfAzimuth);
    float transmissionExponentInput = normalizedPerpendicularDot * 17.0 - 16.780001;

    float hairRoughnessSq = hairRoughness * hairRoughness;
    float twoHairRoughnessSq = hairRoughnessSq + hairRoughnessSq;





    // Primary specular 

    float primaryAzimuthScale = cosHalfAzimuth * 0.997551;
    float primaryEnergyScale = cosHalfAzimuth * 0.25;

    float sinTangentView = sqrt(max(1.0 - TdotV * TdotV, 0.0));
    float primaryShift = primaryAzimuthScale * sinTangentView + TdotV * -0.069943;

    float primaryWidthBase = cosHalfAzimuth * hairRoughnessSq;
    float primaryWidth = primaryWidthBase * 1.414214;
    float primaryNorm = primaryWidthBase * 3.544908;

    float tangentDotSum = TdotL + TdotV;
    float primaryCentered = tangentDotSum + primaryShift * 0.139886;

    float primaryExponent = -0.5 * primaryCentered * primaryCentered / (primaryWidth * primaryWidth);
    float primarySpecular = exp(primaryExponent) / primaryNorm;

    float viewLightHalf = sqrt(clamp(VdotL * 0.5 + 0.5, 0.0, 1.0));
    float oneMinusViewLightHalf = 1.0 - viewLightHalf;

    float primaryFresnel = oneMinusViewLightHalf * oneMinusViewLightHalf;
    primaryFresnel *= primaryFresnel;
    primaryFresnel *= oneMinusViewLightHalf;
    primaryFresnel = primaryFresnel * 0.953479 + 0.046521;

    primarySpecular *= kSpecularStrength;
    primarySpecular *= primaryEnergyScale;
    primarySpecular *= primaryFresnel;
    primarySpecular *= hairSpecularMask * 2.0;

    float primaryVisibility = enableSecondarySpecular ? 1.0 : 1.0 - clamp(-VdotL, 0.0, 1.0);

    vec3 hairSpecularAccum = vec3(primarySpecular * primaryVisibility);







    // Primary transmission

    float secondaryEnabledFloat = enableSecondarySpecular ? 1.0 : 0.0;

    float transmissionGeometryDenominator = cosHalfTangentAngleDifference * 0.36 + 1.19 / cosHalfTangentAngleDifference;

    float transmissionWidth = hairRoughnessSq * 0.5;
    float transmissionNorm = hairRoughnessSq * 1.253314;

    float transmissionCentered = tangentDotSum - 0.035;
    float transmissionExponent = -0.5 * transmissionCentered * transmissionCentered / (transmissionWidth * transmissionWidth);
    float transmissionLobe = exp(transmissionExponent) / transmissionNorm;

    float inverseTransmissionGeometry = 1.0 / transmissionGeometryDenominator;

    float transmissionAzimuthModifier = 1.0 + inverseTransmissionGeometry * (0.6 - normalizedPerpendicularDot * 0.8);
    float modifiedCosHalfAzimuth = cosHalfAzimuth * transmissionAzimuthModifier;

    float transmissionFresnelInput = 1.0 - cosHalfTangentAngleDifference * sqrt(max(1.0 - modifiedCosHalfAzimuth * modifiedCosHalfAzimuth, 0.0));

    float transmissionFresnel = transmissionFresnelInput * transmissionFresnelInput;
    transmissionFresnel *= transmissionFresnel;
    transmissionFresnel *= transmissionFresnelInput;
    transmissionFresnel = transmissionFresnel * 0.953479 + 0.046521;

    float transmissionFresnelWeight = (1.0 - transmissionFresnel) * (1.0 - transmissionFresnel);

    float transmissionColorPower = sqrt(max(1.0 - inverseTransmissionGeometry * modifiedCosHalfAzimuth * inverseTransmissionGeometry * modifiedCosHalfAzimuth, 0.0));
    transmissionColorPower *= 0.5;
    transmissionColorPower /= cosHalfTangentAngleDifference;

    vec3 transmissionColor = pow(abs(diffuseColor), vec3(transmissionColorPower));

    float transmissionAngularWeight = exp(-3.98 - normalizedPerpendicularDot * 3.65);

    float transmissionSpecular = transmissionLobe;
    transmissionSpecular *= kTransmissionStrength;
    transmissionSpecular *= transmissionAngularWeight;
    transmissionSpecular *= transmissionFresnelWeight;

    hairSpecularAccum = vec3(hairSpecularAccum.z) + transmissionColor * transmissionSpecular * secondaryEnabledFloat;




    // Secondary lobe

    float secondaryCentered = tangentDotSum - 0.140;
    float secondaryWidth = twoHairRoughnessSq;
    float secondaryNorm = hairRoughnessSq * 5.013257;

    float secondaryExponent = -0.5 * secondaryCentered * secondaryCentered / (secondaryWidth * secondaryWidth);
    float secondarySpecular = exp(secondaryExponent) / secondaryNorm;

    float secondaryFresnelInput = 1.0 - cosHalfTangentAngleDifference * 0.5;

    float secondaryFresnel = secondaryFresnelInput * secondaryFresnelInput;
    secondaryFresnel *= secondaryFresnel;
    secondaryFresnel *= secondaryFresnelInput;
    secondaryFresnel = secondaryFresnel * 0.953479 + 0.046521;

    float secondaryFresnelWeight = secondaryFresnel * (1.0 - secondaryFresnel) * (1.0 - secondaryFresnel);

    float secondaryColorPower = 0.8 / cosHalfTangentAngleDifference;
    vec3 secondaryColor = pow(abs(diffuseColor), vec3(secondaryColorPower));

    float secondaryAngularWeight = exp(transmissionExponentInput);

    secondarySpecular *= kSecondarySpecularStrength;
    secondarySpecular *= secondaryAngularWeight;
    secondarySpecular *= secondaryFresnelWeight;

    hairSpecularAccum += secondaryColor * secondarySpecular;



     //vec3 kLightColor = vec3(0.8187, 0.8580, 0.7458);


    // Diffuse

    vec3 normalizedViewPerpendicularToTangent = normalize(viewPerpendicularToTangent);

    float tangentDiffuseA = dot(normalizedViewPerpendicularToTangent, lightDirection);
    tangentDiffuseA = clamp((tangentDiffuseA + 1.0) * 0.25, 0.0, 1.0);

    float tangentDiffuseB = 1.0 - abs(TdotL);

    float tangentDiffuse = tangentDiffuseA + (tangentDiffuseB - tangentDiffuseA) * 0.33;

    float normalDiffuse = clamp(dot(finalNormal, lightDirection), 0.0, 1.0);
    normalDiffuse *= kDiffuseStrength;

    tangentDiffuse *= kDiffuseStrength;

    float directDiffuseFactor = normalDiffuse + kNoBackFaceTriangles * (tangentDiffuse - normalDiffuse);

    vec3 directLightColor = lightColor * lightVisibility;

    vec3 directDiffuseLighting = directLightColor * directDiffuseFactor;

    vec3 directSpecularLighting = hairSpecularAccum * 3.141593;
    directSpecularLighting *= lightColor * lightVisibility * hairSpecularMask;

    vec3 directDiffuseColor = diffuseColor * directDiffuseLighting;











    vec3 finalColor = directDiffuseColor + directSpecularLighting;

    float finalAO = texture(hairAutoBakedBaseColorSampler, v_texCoord).a;
    finalColor *= finalAO;

    //finalColor = vec3(transmissionSpecular);

    /*






    if (overrideTangent) {
        cc5Tangent = finalTangent;
    }




    vec3 directDiffuseLighting = vec3(0.0);
    vec3 directSpecularLighting = vec3(0.0);

    
    vec3 characterPosition = vec3(37.0, 31.0, 36.23);
    vec3 relativeLightPosition = vec3(0, 2.2, 0);

    //for (int i = 2; i < 4; i++) {
    {   
        int i = 2;
        Light light = lights[i];
        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;
        
        if (overrideLight) {
            lightPos = characterPosition + relativeLightPosition;
            lightCol = vec3(0.8187, 0.8580, 0.7458);
            lightStrength = 1.4300;
            lightRadius = 5;
        }

        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float attenuation = 1.0 / max(distanceSquared, 1.0);

        float rawShadow = ShadowCalculation(i, lightPos, lightRadius, v_worldPos.xyz, viewPos, n, u_shadowMapArray);

        if (overrideLight) rawShadow = 1.0;

        float shadow = rawShadow * (1.0 - kShadowOpacity) + kShadowOpacity;
        shadow = float(kReceiveShadow) * (shadow - 1.0) + 1.0;

        float lightScalar = lightStrength * attenuation * shadow;

        
        vec3 cc5Specular = CC5HairSpecularPrimarySecondary_NoTransmission(
            cc5Tangent,
            cc5View,
            L,
            hairBaseColor,
            roughness,
            hairSpecularMask,
            kSpecularStrength,
            kSecondarySpecularStrength
        );

        float cc5Diffuse = CC5HairDiffuseScalar(
            cc5Tangent,
            cc5View,
            L,
            n,
            kDiffuseStrength,
            kNoBackFaceTriangles
        );

        vec3 diffuseContribution = hairBaseColor * cc5Diffuse * lightCol * lightScalar;
        vec3 specularContribution = cc5Specular * 3.141593 * lightCol * lightScalar;


        directDiffuseLighting += diffuseContribution;
        directSpecularLighting += specularContribution;
        
    }

    vec3 directLighting = directDiffuseLighting + directSpecularLighting;

    vec2 resolution = vec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
    vec2 screenUV = (vec2(gl_FragCoord.xy) + 0.5) / resolution;

    vec3 indirectDiffuse = vec3(0.0);
    bool u_sampleProbes = false;
    vec3 probeIrradiance = texture(u_indirectDiffuseTexture, screenUV).rgb;

    vec3 diffuseAlbedo = hairBaseColor.rgb * (1.0 - metallic);
    float indirectDiffuseScale = 1.0;

    if (u_sampleProbes) {
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    vec3 finalColor = (directLighting + indirectDiffuse);//* hairAO;

    */

    //finalColor = hairBaseColor;

    
    //finalColor = normalWS;
    //finalColor = v_normal;
    //finalColor = baseHairTangentTS;
    //finalColor = tangentWS;

    //finalColor = cc5Tangent;

    //finalColor = flowMap;
    //finalColor = vec3(hairID);
    //finalColor = vec3(idOffset);
    //finalColor = vec3(baseColor);

    finalColor.rgb += vec3(0.00001);



    LightingOut = vec4(finalColor, 1.0);
}
