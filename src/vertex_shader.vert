#version 150 core


in vec2 position;
in vec3 color_in;
in float sides;

out vec3 vertex_color;
out float vertex_sides;


void main()
{
    gl_Position = vec4( position, 0.0, 1.0 );
    vertex_color = color_in;
    vertex_sides = sides;
}
