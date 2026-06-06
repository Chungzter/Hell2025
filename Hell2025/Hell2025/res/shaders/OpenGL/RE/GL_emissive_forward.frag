#version 460
#extension GL_ARB_bindless_texture : enable

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

    vec3 emissiveMapColor = vec3(1.0);
    
    if (renderItem.emissiveTextureIndex != -1) {
      //  emissiveMapColor = texture(sampler2D(textureSamplers[renderItem.emissiveTextureIndex]), v_uv).rgb;
    }

    vec3 finalColor = emissiveMapColor * vec3(renderItem.emissiveR, renderItem.emissiveG, renderItem.emissiveB);

    EmissiveOut = vec4(finalColor, 1.0);
}