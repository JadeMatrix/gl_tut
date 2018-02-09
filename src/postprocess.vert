#version 150 core


in vec2 position;
in vec2 texture_coord_in;

out vec2 texture_coord;


void main()
{
    texture_coord = texture_coord_in;
    gl_Position = vec4(
        position.x,
        position.y * -1.0,
        0.0,
        1.0
    );
}
