#version 460
#extension GL_ARB_bindless_texture : enable
layout(origin_upper_left) in vec4 gl_FragCoord;
#include "../../common/normal_encoding.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 EmissiveOut;

readonly restrict layout(std430, binding = 0) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = 1) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = 2) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = 3) buffer renderItemsBuffer  { RenderItem renderItems[]; };

in flat int v_globalInstanceIndex;

in vec2 v_uv;

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    vec3 emissiveColor = vec3(renderItem.emissiveR, renderItem.emissiveG, renderItem.emissiveB);

    if (renderItem.emissiveTextureIndex != -1) {
        emissiveColor *= texture(sampler2D(textureSamplers[renderItem.emissiveTextureIndex]), v_uv).rgb;
    }
   
    EmissiveOut = vec4(emissiveColor, 1.0);
}
