#version 450

layout (location = 0) in vec4 inColour;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColour;

layout (set = 1, binding = 0) uniform sampler2D tex1;
layout (set = 1, binding = 1) uniform sampler2D tex2;

void main() {
    vec3 color = mix(texture(tex1, texCoord), texture(tex2, texCoord), 0.25f).xyz;
    outFragColour = vec4(color, 1.0f);
}