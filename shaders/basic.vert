#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vNormal;

layout (location = 0) out vec4 outColour;

void main() {

    gl_Position = vec4(vPosition, 1.0f);

    outColour = vColour;
}