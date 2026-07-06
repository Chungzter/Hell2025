#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/flags.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/util.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/binding_indices.glsl"
#include "../common/Vulkan/push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_UINT_TEXTURES) uniform utexture2D uintTextures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) out vec4 BaseColorMetallicOut;
layout(location = 1) out vec4 NormalXYRoughnessMiscOut;
layout(location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

void WriteMaterialResolveError(vec3 color) {
    BaseColorMetallicOut = vec4(color, 1.0);
    NormalXYRoughnessMiscOut = vec4(0.0, 0.0, 1.0, 1.0);
    VelocityXYOcclusionSubSurfaceOut = vec4(0.0, 0.0, 0.0, 0.0);
}

struct PackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    PackedVertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uint indices[];
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportDataArr[];
};

layout(buffer_reference, scalar) readonly buffer RenderItemBuffer {
    RenderItem renderItems[];
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsMaterialResolve data;
} pushConstant;

float Cross2D(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

vec3 ComputeScreenBarycentrics(vec2 p, vec2 s0, vec2 s1, vec2 s2, vec3 invW) {
    vec2 a = s0 - p;
    vec2 b = s1 - p;
    vec2 c = s2 - p;

    float area = Cross2D(b - a, c - a);
    if (abs(area) < 1e-20) {
        return vec3(1.0, 0.0, 0.0);
    }

    float u = Cross2D(b, c) / area;
    float v = Cross2D(c, a) / area;
    float w = Cross2D(a, b) / area;

    float interpW = u * invW.x + v * invW.y + w * invW.z;
    if (abs(interpW) < 1e-20) {
        return vec3(1.0, 0.0, 0.0);
    }

    return vec3(u * invW.x, v * invW.y, w * invW.z) / interpW;
}

uint ComputeViewportIndexFromSplitscreenModeVK(ivec2 px, ivec2 size, int mode) {
    if (px.x < 0 || px.y < 0 || px.x >= size.x || px.y >= size.y) {
        return 0u;
    }

    if (mode == 0) {
        return 0u;
    }

    uint x = uint(px.x >= (size.x / 2));
    uint y = uint(px.y <  (size.y / 2));

    if (mode == 1) {
        return y;
    }

    if (mode == 2) {
        return x + (y << 1);
    }

    return 0u;
}

void main() {
    VertexBuffer vertexBuffer = VertexBuffer(pushConstant.data.vertexBufferDeviceAddress);
    IndexBuffer indexBuffer = IndexBuffer(pushConstant.data.indexBufferDeviceAddress);
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.viewportDataDeviceAddress);
    RenderItemBuffer renderItemBuffer = RenderItemBuffer(pushConstant.data.renderItemsDeviceAddress);
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.rendererDataDeviceAddress);
    MaterialBuffer materialBuffer = MaterialBuffer(pushConstant.data.materialsDeviceAddress);
    
    ivec2 outputImageSize = textureSize(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    ivec2 px = ivec2(gl_FragCoord.xy);

    uint viewportIndex = ComputeViewportIndexFromSplitscreenModeVK(px, outputImageSize, rendererDataBuffer.rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];

    vec2 viewportSize = vec2(viewportData.width, viewportData.height);
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);

    uvec4 visibilityData = uvec4(texelFetch(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).xy, 0u, 0u);
    
    uint globalInstanceIndex = visibilityData.x;
    uint primitiveID = visibilityData.y;

    RenderItem renderItem = renderItemBuffer.renderItems[globalInstanceIndex];
    Material material = materialBuffer.materials[renderItem.materialIndex];
    uint triangleIndexOffset = renderItem.baseIndex + (primitiveID * 3);

    if (triangleIndexOffset + 2u >= pushConstant.data.indexCount) {
        WriteMaterialResolveError(vec3(1.0, 0.0, 1.0));
        return;
    }

    uint i0 = indexBuffer.indices[triangleIndexOffset + 0] + renderItem.baseVertex;
    uint i1 = indexBuffer.indices[triangleIndexOffset + 1] + renderItem.baseVertex;
    uint i2 = indexBuffer.indices[triangleIndexOffset + 2] + renderItem.baseVertex;

    if (i0 >= pushConstant.data.vertexCount || i1 >= pushConstant.data.vertexCount || i2 >= pushConstant.data.vertexCount) {
        WriteMaterialResolveError(vec3(0.0, 1.0, 1.0));
        return;
    }
    
    PackedVertex v0 = vertexBuffer.vertices[i0];
    PackedVertex v1 = vertexBuffer.vertices[i1];
    PackedVertex v2 = vertexBuffer.vertices[i2];

    // Position from depth reconstruction
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    vec3 worldPos = ReconstructWorldPos(viewportUV, depth, viewportData.inverseProjectionViewReverseZ);

    // Transform vertices to world space
    mat4 modelMatrix = renderItem.modelMatrix;
    vec3 ws0 = (modelMatrix * vec4(v0.vx, v0.vy, v0.vz, 1.0)).xyz;
    vec3 ws1 = (modelMatrix * vec4(v1.vx, v1.vy, v1.vz, 1.0)).xyz;
    vec3 ws2 = (modelMatrix * vec4(v2.vx, v2.vy, v2.vz, 1.0)).xyz;

    // Calculate world space edges
    vec3 viewDir = normalize(worldPos - viewportData.viewPos.xyz);
    vec3 e1 = ws1 - ws0;
    vec3 e2 = ws2 - ws0;
    vec3 geoNormal = normalize(cross(e1, e2));
    bool isFrontFacing = dot(geoNormal, viewDir) <= 0.0;

    // Project vertices to NDC for stable screen space barycentrics
    vec4 clip0 = viewportData.projectionViewReverseZ * vec4(ws0, 1.0);
    vec4 clip1 = viewportData.projectionViewReverseZ * vec4(ws1, 1.0);
    vec4 clip2 = viewportData.projectionViewReverseZ * vec4(ws2, 1.0);

    vec2 viewportNDC = ViewportNDCFromPixel_VK(px, outputImageSize, viewportData);

    vec2 s0 = clip0.xy / clip0.w;
    vec2 s1 = clip1.xy / clip1.w;
    vec2 s2 = clip2.xy / clip2.w;

    vec3 invW = vec3(1.0 / clip0.w, 1.0 / clip1.w, 1.0 / clip2.w);

    vec2 pixelStep = 2.0 / viewportSize;

    vec3 bary  = ComputeScreenBarycentrics(viewportNDC, s0, s1, s2, invW);
    vec3 baryX = ComputeScreenBarycentrics(viewportNDC + vec2(pixelStep.x, 0.0), s0, s1, s2, invW);
    vec3 baryY = ComputeScreenBarycentrics(viewportNDC + vec2(0.0, -pixelStep.y), s0, s1, s2, invW);

    vec2 uv0 = vec2(v0.u, v0.v);
    vec2 uv1 = vec2(v1.u, v1.v);
    vec2 uv2 = vec2(v2.u, v2.v);

    vec2 uv  = uv0 * bary.x  + uv1 * bary.y  + uv2 * bary.z;
    vec2 uvX = uv0 * baryX.x + uv1 * baryX.y + uv2 * baryX.z;
    vec2 uvY = uv0 * baryY.x + uv1 * baryY.y + uv2 * baryY.z;

    vec2 dPdx = uvX - uv;
    vec2 dPdy = uvY - uv;
    dPdx = clamp(dPdx, vec2(-1.0), vec2(1.0));
    dPdy = clamp(dPdy, vec2(-1.0), vec2(1.0));

    mat4 normalMatrix = transpose(renderItem.inverseModelMatrix);

    vec3 n0 = normalize(normalMatrix * vec4(v0.nx, v0.ny, v0.nz, 0.0)).xyz;
    vec3 n1 = normalize(normalMatrix * vec4(v1.nx, v1.ny, v1.nz, 0.0)).xyz;
    vec3 n2 = normalize(normalMatrix * vec4(v2.nx, v2.ny, v2.nz, 0.0)).xyz;
    vec3 n  = normalize(n0 * bary.x  + n1 * bary.y  + n2 * bary.z);

    vec3 nX = normalize(n0 * baryX.x + n1 * baryX.y + n2 * baryX.z);
    vec3 nY = normalize(n0 * baryY.x + n1 * baryY.y + n2 * baryY.z);

    vec3 t0 = normalize(modelMatrix * vec4(v0.tx, v0.ty, v0.tz, 0.0)).xyz;
    vec3 t1 = normalize(modelMatrix * vec4(v1.tx, v1.ty, v1.tz, 0.0)).xyz;
    vec3 t2 = normalize(modelMatrix * vec4(v2.tx, v2.ty, v2.tz, 0.0)).xyz;
    vec3 t = normalize(t0 * bary.x + t1 * bary.y + t2 * bary.z);

    //if (!isFrontFacing) {
    //    n = -n;
    //    nX = -nX;
    //    nY = -nY;
    //}

    // Check this is not needed when u get floor rendering

    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);

    uint baseColorTextureIndex = uint(material.basecolor);
    uint normalTextureIndex = uint(material.normal);
    uint rmaTextureIndex = uint(material.rma);
    vec4 baseColor = textureGrad(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), uv, dPdx, dPdy);
    vec3 normalMap = textureGrad(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), uv, dPdx, dPdy).rgb;
    vec4 rma = textureGrad(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), uv, dPdx, dPdy).rgba;

    float roughness = rma.r;
    float metallic  = rma.g;
    float ao = rma.b;

    normalMap = normalMap * 2.0 - 1.0;
    vec3 normal = normalize(tbn * normalMap);

    vec3 dNdx = nX - n;
    vec3 dNdy = nY - n;
    float variance = (dot(dNdx, dNdx) + dot(dNdy, dNdy)) * 0.1591549;
    roughness = sqrt(clamp(roughness * roughness + min(variance, 0.18), 0.0, 1.0));

    vec4 localPos = renderItem.inverseModelMatrix * vec4(worldPos, 1.0);
    vec4 currPos = viewportData.projectionViewReverseZ * vec4(worldPos, 1.0);
    vec4 prevPos = viewportData.prevProjectionViewReverseZ * renderItem.prevModelMatrix * localPos;

    vec2 currNDC = currPos.xy / currPos.w;
    vec2 prevNDC = prevPos.xy / prevPos.w;
    vec2 velocity = (currNDC - prevNDC) * 0.5;

    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    NormalXYRoughnessMiscOut.rg = EncodeNormal(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = EncodeMiscFlags(renderItem.miscFlags);

    VelocityXYOcclusionSubSurfaceOut.rg = velocity;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = 0.0;

   // BaseColorMetallicOut.rgb = clamp(normal, vec3(0), vec3(1));
   //if (viewportIndex == 0) {
   //    BaseColorMetallicOut.rgb = vec3(1,0,0);
   //}
   //else if (viewportIndex == 1) {
   //    BaseColorMetallicOut.rgb = vec3(0,1,0);
   //}
   //else if (viewportIndex == 2) {
   //    BaseColorMetallicOut.rgb = vec3(0,0,1);
   //}
   //else if (viewportIndex == 3) {
   //    BaseColorMetallicOut.rgb = vec3(0,1,1);
   //}
}
