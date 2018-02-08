// See https://gist.github.com/cbmeeks/5587a11e7856baf819b7
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#else
    #include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

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
}


int main( int argc, char* argv[] )
{
    SDL_Init( SDL_INIT_EVERYTHING );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3                           );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2                           );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE         , 8                           );
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
            
            // GLuint test_vb;
            // std::cout << "glGenBuffers: " << ( unsigned long )glGenBuffers << std::endl;
            // glGenBuffers( 1, &test_vb );
            // std::cout << "test vertex buffer: " << test_vb << std::endl;
            
            float triangle_vertices[] = {
                 0.0f,  0.5f,
                 0.5f, -0.5f,
                -0.5f, -0.5f
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
            
            GLuint vertex_shader = compile_shader_from_file(
                GL_VERTEX_SHADER,
                "../src/vertex_shader.vert"
            );
            GLuint fragment_shader = compile_shader_from_file(
                GL_FRAGMENT_SHADER,
                "../src/fragment_shader.frag"
            );
            if( vertex_shader && fragment_shader )
            {
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
                
                GLint position_attr = glGetAttribLocation(
                    shader_program,
                    "position"
                );
                glVertexAttribPointer(
                    position_attr,  // Data source
                    2,              // Components per element
                    GL_FLOAT,       // Component type
                    GL_FALSE,       // Components should be normalized
                    0,              // Component stride in bytes (0 = packed)
                    0               // Component offset within stride
                );
                glEnableVertexAttribArray( position_attr );
                
                SDL_Event window_event;
                while( true )
                {
                    if( SDL_PollEvent( &window_event ) )
                    {
                        if( window_event.type == SDL_QUIT )
                            break;
                        else if(
                            SDL_GetWindowFlags( sdl_window ) & (
                                  SDL_WINDOW_FULLSCREEN
                                | SDL_WINDOW_FULLSCREEN_DESKTOP
                            )
                            && window_event.type == SDL_KEYUP
                            && window_event.key.keysym.sym == SDLK_ESCAPE
                        )
                            break;
                    }
                    
                    // Update //////////////////////////////////////////////////
                    {
                        glUseProgram( shader_program );
                        glBindVertexArray( program_vao );
                        
                        GLuint triangle_color = glGetUniformLocation(
                            shader_program,
                            "triangle_color"
                        );
                        glUniform3f( triangle_color, 0.0f, 1.0f, 0.0f );
                        
                        glDrawArrays(
                            GL_TRIANGLES,   // Type of primitive
                            0,              // Start primitive
                            3               // Number of primitives
                        );
                    }
                    
                    SDL_GL_SwapWindow( sdl_window );
                }
            }
            else
            {
                std::cerr
                    << "failed to compile a shader, exiting..."
                    << std::endl
                ;
            }
        }
        SDL_DestroyWindow( sdl_window );
        SDL_GL_DeleteContext( context );
    }
    SDL_Quit();
    
    return 0;
}
