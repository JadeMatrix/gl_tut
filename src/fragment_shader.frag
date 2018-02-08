#version 150 core


in vec3 color;

out vec4 color_out;


void main()
{
    // color_out = vec4( color, 1.0 );
    color_out = vec4(
        1.0 - color.r,
        1.0 - color.g,
        1.0 - color.b,
        1.0
    );
}
