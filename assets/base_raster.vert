#version 460 core

layout (location = 0) in vec3 i_position;
layout (location = 1) in uint i_colour_index;

layout (location = 0) out vec4 o_colour;

uniform mat4 u_vp;
uniform mat4 u_model;

layout (std430, binding = 0) readonly buffer ColoursBuffer {
    vec4 data[];
} colours;

void main() {
    gl_Position = u_vp * u_model * vec4(i_position, 1.0f);
    o_colour = colours.data[i_colour_index];
}
