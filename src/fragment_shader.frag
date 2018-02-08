#version 150 core


in vec3 color;
in vec2 texture_coord;

out vec4 color_out;

uniform sampler2D texture_A;
uniform sampler2D texture_B;


void main()
{
    vec4 color_A = texture(
        texture_A,
        vec2(
                  texture_coord.x,
            1.0 - texture_coord.y
        )
    );
    vec4 color_B = texture(
        texture_B,
        vec2(
                  texture_coord.x,
            1.0 - texture_coord.y
        )
    );
    color_out = mix(
        color_A,
        color_B,
        texture_coord.x
    );
    color_out *= vec4(
        1.0 - color.r,
        1.0 - color.g,
        1.0 - color.b,
        1.0
    );
}
