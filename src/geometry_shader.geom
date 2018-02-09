#version 150 core


layout( points ) in;
layout( line_strip, max_vertices = 64 ) out;

in vec3 vertex_color[];
in float vertex_sides[];

out vec3 fragment_color;

const float PI = 3.1415926;


void main()
{
    fragment_color = vertex_color[ 0 ];
    
    for( int i = 0; i <= vertex_sides[ 0 ]; ++i )
    {
        // Angle between each side in radians
        float angle = PI * 2.0 / vertex_sides[ 0 ] * i;
        
        // Offset from center of point (0.3 to accommodate for aspect ratio)
        gl_Position = gl_in[ 0 ].gl_Position + vec4(
             cos( angle ) * 0.3,
            -sin( angle ) * 0.4,
            0.0,
            0.0
        );
        
        EmitVertex();
    }
    
    EndPrimitive();
}
