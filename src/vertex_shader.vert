#version 150 core


in vec2 position;
in vec3 color_in;

out vec3 color;


void main()
{
    // gl_Position = vec4( position, 0.0, 1.0 );
    gl_Position = vec4( position.x, position.y * -1.0, 0.0, 1.0 );
    color = color_in;
}
