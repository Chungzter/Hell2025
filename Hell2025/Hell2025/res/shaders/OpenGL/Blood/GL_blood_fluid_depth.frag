#version 460
layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out float DepthOut;
in vec4 v_viewSpacePosition;

void main() {
    DepthOut = v_viewSpacePosition.z;
}
