#version 450
layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 3) out vec4 EmissiveOut;

in vec4 WorldPos;
in vec3 Normal;

void main() {
	BaseColorOut = vec4(0.5, 0.0, 0.0, 1.0); 
    NormalOut = vec4(normalize(Normal), 1);
	RMAOut = vec4(0.015 , 0.54, 1, 1);
	EmissiveOut = vec4(0, 0, 0, 1);
}
