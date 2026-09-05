#version 460 core

layout (location = 0) in vec3 i_position;

layout (location = 1) in vec3 i_instance_position;
layout (location = 2) in uint i_instance_colour;

layout (location = 0) out vec4 o_colour;

uniform mat4 u_mvp;
uniform mat4 u_chunk;

layout (std430, binding = 0) readonly buffer ColoursBuffer {
    vec4 data[];
} colours;

void main() {
    gl_Position = u_mvp * u_chunk * vec4(i_position + i_instance_position, 1.0f);
    o_colour = colours.data[i_instance_colour];
}
