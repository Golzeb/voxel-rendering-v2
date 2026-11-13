#version 460 core

layout (location = 0) in vec4 i_colour;

out vec4 frag_colour;

void main() {
    frag_colour = i_colour;
}
