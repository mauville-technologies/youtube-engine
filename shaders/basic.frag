#version 450

layout (location = 0) in vec3 inColour;

layout (location = 0) out vec4 outFragColour;

void main() {
    outFragColour = vec4(inColour, 1.0f);
}