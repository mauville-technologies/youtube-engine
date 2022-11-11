#version 450

layout (location = 0) in vec4 inColour;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColour;

// layout (set = 1, binding = 0) uniform sampler2D diffuse;

void main() {
    // vec3 color = texture(diffuse, texCoord).xyz;
    // outFragColour = vec4(color, 1.0);
    outFragColour = inColour;
}