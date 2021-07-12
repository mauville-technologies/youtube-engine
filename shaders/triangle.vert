#version 450

layout (location = 0) out vec3 outColour;

void main() {
    // const array of positions for the triangle
    const vec3 positions[3] = vec3[3] (
        vec3( 1.f,  1.f, 0.f),
        vec3(-1.f,  1.f, 0.f),
        vec3( 0.f, -1.f, 0.f)
    );
    const vec3 colours[3] = vec3[3] (
        vec3(1.f, 0.f, 0.f),
        vec3(0.f, 1.f, 0.f),
        vec3(0.f, 0.f, 1.f)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);

    outColour = colours[gl_VertexIndex];
}