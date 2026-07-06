#version 460  core
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;

layout(std430, binding = 4) readonly buffer renderItemsBuffer {
    RenderItem renderItems[];
};

uniform mat4 u_projectionView;
uniform mat4 u_modelMatrix;
uniform bool u_useInstanceData = false;

void main() {

    // Regular render items
    if (u_useInstanceData) {
        int instanceOffset = gl_BaseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
        int globalInstanceIndex = instanceOffset + gl_InstanceID;
    
        RenderItem renderItem = renderItems[globalInstanceIndex]; 
        mat4 modelMatrix = renderItem.modelMatrix;

        gl_Position = u_projectionView * modelMatrix * vec4(vPosition, 1.0);
    }

    // Height maps, and house render items
    else {
        gl_Position = u_projectionView * u_modelMatrix * vec4(vPosition, 1.0);
    }
}  