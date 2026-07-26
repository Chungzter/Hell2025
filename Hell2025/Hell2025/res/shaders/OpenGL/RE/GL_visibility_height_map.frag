#version 460

layout(location = 0) out uvec2 VisBufferOut;

layout(location = 0) flat in int v_sceneRenderItemIndex;
layout(location = 2) in vec3 v_worldPosition;

uniform float u_discardHeight = 0.01;

void main() {
    if (v_worldPosition.y < u_discardHeight) {
        discard;
    }

    VisBufferOut.x = uint(v_sceneRenderItemIndex);
    VisBufferOut.y = uint(gl_PrimitiveID);
}
