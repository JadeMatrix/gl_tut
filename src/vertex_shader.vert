#version 150 core


in vec2 position;
in vec3 color_in;
in vec2 texture_coord_in;

out vec3 color;
out vec2 texture_coord;

uniform mat4  transform_model;
uniform mat4  transform_view;
uniform mat4  transform_projection;
uniform float time_absolute;
uniform float time_delta;


void main()
{
    mat4 rotation;
    float sin_val = sin( radians( time_absolute ) * 10.0 );
    float cos_val = cos( radians( time_absolute ) * 10.0 );
    rotation[ 0 ] = vec4(  cos_val, sin_val, 0.0, 0.0 );
    rotation[ 1 ] = vec4( -sin_val, cos_val, 0.0, 0.0 );
    rotation[ 2 ] = vec4(      0.0,     0.0, 1.0, 0.0 );
    rotation[ 3 ] = vec4(      0.0,     0.0, 0.0, 1.0 );
    
    gl_Position = (
          transform_projection
        * transform_view
        * rotation
        * transform_model
        * vec4(
            position.x,
            position.y * -1.0,
            0.0,
            1.0
        )
    );
    
    color = color_in;
    texture_coord = texture_coord_in;
}
