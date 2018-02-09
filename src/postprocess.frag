#version 150 core


in vec2 texture_coord;

out vec4 color_out;

uniform sampler2D framebuffer;


void main()
{
    // Invert //////////////////////////////////////////////////////////////////
    
    // color_out = texture( framebuffer, texture_coord );
    // color_out = vec4( 1.0 ) - color_out;
    
    
    // Greyscale ///////////////////////////////////////////////////////////////
    
    // color_out = texture( framebuffer, texture_coord );
    
    // // // Unweighted
    // // float average = (
    // //       color_out.r
    // //     + color_out.g
    // //     + color_out.b
    // // ) / 3.0;
    
    // // Weighted:
    // float average = (
    //       color_out.r * 0.2126
    //     + color_out.g * 0.7152
    //     + color_out.b * 0.0722
    // );
    
    // color_out = vec4( vec3( average ), 1.0 );
    
    
    // Sobel ///////////////////////////////////////////////////////////////////
    
    // vec4 top          = texture( framebuffer, vec2( texture_coord.x              , texture_coord.y + 1.0 / 200.0 ) );
    // vec4 bottom       = texture( framebuffer, vec2( texture_coord.x              , texture_coord.y - 1.0 / 200.0 ) );
    // vec4 left         = texture( framebuffer, vec2( texture_coord.x - 1.0 / 300.0, texture_coord.y               ) );
    // vec4 right        = texture( framebuffer, vec2( texture_coord.x + 1.0 / 300.0, texture_coord.y               ) );
    // vec4 top_left     = texture( framebuffer, vec2( texture_coord.x - 1.0 / 300.0, texture_coord.y + 1.0 / 200.0 ) );
    // vec4 top_right    = texture( framebuffer, vec2( texture_coord.x + 1.0 / 300.0, texture_coord.y + 1.0 / 200.0 ) );
    // vec4 bottom_left  = texture( framebuffer, vec2( texture_coord.x - 1.0 / 300.0, texture_coord.y - 1.0 / 200.0 ) );
    // vec4 bottom_right = texture( framebuffer, vec2( texture_coord.x + 1.0 / 300.0, texture_coord.y - 1.0 / 200.0 ) );
    // vec4 sx = -top_left - 2 * left - bottom_left + top_right   + 2 * right  + bottom_right;
    // vec4 sy = -top_left - 2 * top  - top_right   + bottom_left + 2 * bottom + bottom_right;
    // color_out = sqrt( sx * sx + sy * sy );
    
    
    // Box blur ////////////////////////////////////////////////////////////////
    
    float blur_size_h = 1.0 / 300.0;
    float blur_size_v = 1.0 / 200.0;
    color_out = vec4( 0.0 );
    
    for( int x = -4; x <= 4; ++x )
        for( int y = -4; y <= 4; ++y )
            color_out += texture(
                framebuffer,
                vec2(
                    texture_coord.x + x * blur_size_h,
                    texture_coord.y + y * blur_size_v
                )
            ) / 81.0;
}
