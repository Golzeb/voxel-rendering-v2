#version 460 core

layout (location = 0) in vec4 i_colour;

out vec4 frag_color;

void main() {
    frag_color = i_colour;
}
