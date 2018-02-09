#version 150 core


in vec3 color;
in vec2 texture_coord;

out vec4 color_out;

uniform sampler2D texture_A;
uniform sampler2D texture_B;

uniform float time_absolute;
uniform float time_delta;


void main()
{
    vec4 color_A = texture(
        texture_A,
        vec2(
                  texture_coord.x,
            1.0 - texture_coord.y
        )
    );
    // vec4 color_B = texture(
    //     texture_B,
    //     vec2(
    //               texture_coord.x,
    //         1.0 - texture_coord.y
    //     )
    // );
    vec4 color_B = texture(
        texture_B,
        vec2(
            texture_coord.x + sin(
                texture_coord.y * 60.0
                + time_absolute * 2.0
            ) / 30.0,
            1.0 - texture_coord.y
        )
    );
    color_out = mix(
        color_A,
        color_B,
        // texture_coord.x
        ( sin( time_absolute * 2.0 ) + 1.0 ) / 2.0
    );
    // color_out *= vec4(
    //     1.0 - color.r,
    //     1.0 - color.g,
    //     1.0 - color.b,
    //     1.0
    // );
    
    float offset = radians( 120.0 );
    color_out *= vec4(
        ( sin( time_absolute + 0 * offset ) + 1.0 ) / 2.0,
        ( sin( time_absolute + 1 * offset ) + 1.0 ) / 2.0,
        ( sin( time_absolute + 2 * offset ) + 1.0 ) / 2.0,
        1.0
    );
}
