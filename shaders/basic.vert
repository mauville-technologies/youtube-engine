#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vNormal;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec2 texCoord;

layout(set = 0, binding = 0) uniform ModelData {
    mat4 model;
} mod;

layout(set = 0, binding = 1) uniform CameraData {
    mat4 view;
    mat4 proj;
} camera;

void main() {
    gl_Position = camera.proj * camera.view * mod.model * vec4(vPosition, 1.0f);

    outColour = vColour;
    texCoord = vTexCoord;
}