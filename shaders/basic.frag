#version 450

layout (location = 0) in vec4 inColour;

layout (location = 0) out vec4 outFragColour;

void main() {
    outFragColour = inColour;
}