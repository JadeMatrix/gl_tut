// See https://gist.github.com/cbmeeks/5587a11e7856baf819b7
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#else
    #include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

#include <fstream>
#include <iostream>
#include <string>
// #include <thread>   // std::this_thread::yield()


namespace
{
    GLuint compile_shader_from_file( GLenum shader_type, std::string filename )
    {
        std::filebuf source_file;
        source_file.open( filename, std::ios_base::in );
        if( !source_file.is_open() )
        {
            std::cerr
                << "could not open shader source file \""
                << filename
                << "\""
                << std::endl
            ;
            return 0x00;
        }
        
        std::string source = std::string(
            std::istreambuf_iterator< char >( &source_file ),
            {}
        );
        auto source_c_strin = source.c_str();
        
        GLuint shader = glCreateShader( shader_type );
        glShaderSource( shader, 1, &source_c_strin, NULL );
        glCompileShader( shader );
        
        GLint status;
        glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
        if( status != GL_TRUE )
        {
            char log_buffer[ 1024 ];
            glGetShaderInfoLog(
                shader,
                1024,
                NULL,
                log_buffer
            );
            std::cerr
                << "compiling shader\""
                << filename
                << "\" failed:"
                << std::endl
                << log_buffer
                << std::endl
            ;
            
            glDeleteShader( shader );
            shader = 0x00;
        }
        
        return shader;
    }
    
    bool load_bound_texture( std::string filename )
    {
        SDL_Surface* sdl_surface = IMG_Load(
            filename.c_str()
        );
        if( !sdl_surface )
        {
            std::cerr
                << "failed to load texture \""
                << filename
                << "\": "
                << IMG_GetError()
                << std::endl
            ;
            return false;
        }
        // else
        //     std::cout
        //         << "loaded texture \""
        //         << filename
        //         << "\": w="
        //         << sdl_surface -> w
        //         << " h="
        //         << sdl_surface -> h
        //         << " bpp="
        //         << ( int )( sdl_surface -> format -> BytesPerPixel )
        //         << std::endl
        //     ;
        
        GLenum texture_mode = (
            sdl_surface -> format -> BytesPerPixel == 4 ?
            GL_RGBA : GL_RGB
        );
        glTexImage2D(   // Loads starting at 0,0 as bottom left
            GL_TEXTURE_2D,
            0,                      // LoD, 0 = base
            texture_mode,           // Internal format
            sdl_surface -> w,       // Width
            sdl_surface -> h,       // Height
            0,                      // Border; must be 0
            texture_mode,           // Incoming format
            GL_UNSIGNED_BYTE,       // Pixel type
            sdl_surface -> pixels   // Pixel data
        );
        SDL_FreeSurface( sdl_surface );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
        float texture_border_color[] = { 1.0f, 0.0f, 0.0f };
        glTexParameterfv(
            GL_TEXTURE_2D,
            GL_TEXTURE_BORDER_COLOR,
            texture_border_color
        );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glGenerateMipmap( GL_TEXTURE_2D );
        
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        return true;
    }
}


int main( int argc, char* argv[] )
{
    // Initialization //////////////////////////////////////////////////////////
    
    
    SDL_Init( SDL_INIT_EVERYTHING );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3                           );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2                           );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE         , 8                           );
    IMG_Init( IMG_INIT_PNG );
    {
        SDL_Window* sdl_window = SDL_CreateWindow(
            "OpenGL",
            SDL_WINDOWPOS_CENTERED, // 100,
            SDL_WINDOWPOS_CENTERED, // 100,
            800,
            600,
            SDL_WINDOW_OPENGL // | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN
        );
        
        SDL_GLContext context = SDL_GL_CreateContext( sdl_window );
        {
            // Run GLEW stuff _after_ creating SDL/GL context
        #ifndef __APPLE__
            glewExperimental = GL_TRUE;
            if( glewInit() != GLEW_OK )
            {
                std::cerr << "failed to initialize GLEW" << std::endl;
                return -1;
            }
        #endif
            
            
            // Compile shaders /////////////////////////////////////////////////
            
            
            GLuint vertex_shader = compile_shader_from_file(
                GL_VERTEX_SHADER,
                "../src/vertex_shader.vert"
            );
            GLuint fragment_shader = compile_shader_from_file(
                GL_FRAGMENT_SHADER,
                "../src/fragment_shader.frag"
            );
            
            if( !vertex_shader || !fragment_shader )
            {
                std::cerr
                    << "failed to compile a shader, exiting..."
                    << std::endl
                ;
                return -1;
            }
            
            
            // Create shader program ///////////////////////////////////////////
            
            
            GLuint program_vao;
            glGenVertexArrays( 1, &program_vao );
            glBindVertexArray( program_vao );
            
            GLuint shader_program = glCreateProgram();
            glAttachShader( shader_program,   vertex_shader );
            glAttachShader( shader_program, fragment_shader );
            
            // glBindFragDataLocation( shader_program, 0, "outColor" );
            // Use glDrawBuffers when rendering to multiple buffers, because
            // only the first output will be enabled by default.
            
            // Dereference shaders
            glDeleteShader(   vertex_shader );
            glDeleteShader( fragment_shader );
            
            glLinkProgram( shader_program );
            glUseProgram(  shader_program );
            
            
            // Load textures ///////////////////////////////////////////////////
            
            
            GLuint textures[ 2 ];
            glGenTextures( 2, textures );
            
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, textures[ 0 ] );
            if( !load_bound_texture(
                "../resources/textures/rgb.png"
            ) )
                return -1;
            glUniform1i(
                glGetUniformLocation( shader_program, "texture_A"),
                0
            );
            
            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_2D, textures[ 1 ] );
            if( !load_bound_texture(
                "../resources/textures/b&w.png"
            ) )
                return -1;
            glUniform1i(
                glGetUniformLocation( shader_program, "texture_B"),
                1
            );
            
            
            // Create shader input data ////////////////////////////////////////
            
            
            float triangle_vertices[] = {
            //  Position ---| Color ----------| Texture --|
            //      X,     Y,    R,    G,    B,    S,    T
                -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //    top left
                 0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //    top right
                 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // bottom right
                -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // bottom left
            };
            GLuint triangle_vbo;
            glGenBuffers( 1, &triangle_vbo );
            glBindBuffer( GL_ARRAY_BUFFER, triangle_vbo );
            glBufferData(
               GL_ARRAY_BUFFER,
               sizeof( triangle_vertices ),
               triangle_vertices,
               GL_STATIC_DRAW // GL_DYNAMIC_DRAW, GL_STREAM_DRAW
            );
            
            GLuint triangle_elements[] = {
                0, 1, 2,
                2, 3, 0
            };
            GLuint triangle_ebo;
            glGenBuffers( 1, &triangle_ebo );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, triangle_ebo );
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sizeof( triangle_elements ),
                triangle_elements,
                GL_STATIC_DRAW
            );
            
            
            // Configure shader program attributes /////////////////////////////
            
            
            GLint position_attr = glGetAttribLocation(
                shader_program,
                "position"
            );
            glEnableVertexAttribArray( position_attr );
            glVertexAttribPointer(
                position_attr,       // Data source
                2,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                7 * sizeof( float ), // Component stride in bytes (0 = packed)
                NULL                 // Component offset within stride
            );
            
            GLint color_attr = glGetAttribLocation(
                shader_program,
                "color_in"
            );
            glEnableVertexAttribArray( color_attr );
            glVertexAttribPointer(
                color_attr,          // Data source
                3,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                7 * sizeof( float ), // Component stride in bytes (0 = packed)
                ( void* )( 2 * sizeof( float ) )
                                     // Component offset within stride
            );
            
            GLint texture_coord_attr = glGetAttribLocation(
                shader_program,
                "texture_coord_in"
            );
            glEnableVertexAttribArray( texture_coord_attr );
            glVertexAttribPointer(
                texture_coord_attr,  // Data source
                3,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                7 * sizeof( float ), // Component stride in bytes (0 = packed)
                ( void* )( 5 * sizeof( float ) )
                                     // Component offset within stride
            );
            
            
            // Drawing & event handling ////////////////////////////////////////
            
            
            glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
            
            SDL_Event window_event;
            while( true )
            {
                if( SDL_PollEvent( &window_event ) )
                {
                    if( window_event.type == SDL_QUIT )
                        break;
                    else if(
                        window_event.type == SDL_KEYUP
                        && window_event.key.keysym.sym == SDLK_ESCAPE
                        && SDL_GetWindowFlags( sdl_window ) & (
                              SDL_WINDOW_FULLSCREEN
                            | SDL_WINDOW_FULLSCREEN_DESKTOP
                        )
                    )
                        break;
                }
                
                // Update //////////////////////////////////////////////////////
                {
                    glClear(
                          GL_COLOR_BUFFER_BIT
                        | GL_DEPTH_BUFFER_BIT
                        | GL_STENCIL_BUFFER_BIT
                    );
                    
                    glUseProgram( shader_program );
                    glBindVertexArray( program_vao );
                    
                    glDrawElements(
                        GL_TRIANGLES,       // Type of primitive
                        6,                  // Number of elements
                        GL_UNSIGNED_INT,    // Type of element
                        0                   // Starting at element
                    );
                }
                
                SDL_GL_SwapWindow( sdl_window );
            }
            
            
            // Cleanup /////////////////////////////////////////////////////////
            
            
            glDeleteProgram( shader_program );
            
            glDeleteBuffers( 1, &triangle_vbo );
            glDeleteBuffers( 1, &triangle_ebo );
        }
        SDL_DestroyWindow( sdl_window );
        SDL_GL_DeleteContext( context );
    }
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
