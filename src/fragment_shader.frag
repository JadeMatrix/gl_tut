#version 150 core


in vec3 fragment_color;

out vec4 color_out;


void main()
{
    color_out = vec4( fragment_color, 1.0 );
}
