// See https://gist.github.com/cbmeeks/5587a11e7856baf819b7
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#else
    #include <GL/glew.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


namespace gl_tut
{
    class SDL_manager
    {
    public:
        SDL_manager()
        {
            if( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
                throw std::runtime_error(
                    "unable to initialize SDL2: "
                    + std::string( SDL_GetError() )
                );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3                           );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2                           );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE );
            SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE         , 8                           );
            
            int img_flags_in  = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
            int img_flags_out = IMG_Init( img_flags_in );
            if( img_flags_in != img_flags_out )
            {
                std::string img_error_string =
                    "failed to initialize SDL2-image ";
                if( img_flags_out & IMG_INIT_JPG )
                    img_error_string += "JPG";
                else if( img_flags_out & IMG_INIT_PNG )
                    img_error_string += "PNG";
                else if( img_flags_out & IMG_INIT_TIF )
                    img_error_string += "TIF";
                img_error_string += (
                    " support: "
                    + std::string( IMG_GetError() )
                    + " (note IMG error string is not always meaningful when IMG_Init() fails)"
                );
                SDL_Quit();
                throw std::runtime_error( img_error_string.c_str() );
            }
        }
        ~SDL_manager()
        {
            IMG_Quit();
            SDL_Quit();
        }
    };
    
    class SDL_window
    {
    public:
        SDL_Window*   sdl_window;
        SDL_GLContext  gl_context;
        
        SDL_window(
            const std::string& title,
            int x, int y,
            int w, int h,
            Uint32 flags
        )
        {
            sdl_window = SDL_CreateWindow(
                title.c_str(),
                x, y,
                w, h,
                flags
            );
            if( sdl_window == nullptr )
                throw std::runtime_error(
                    "failed to create SDL2 window: "
                    + std::string( SDL_GetError() )
                );
            
            gl_context = SDL_GL_CreateContext( sdl_window );
            if( gl_context == nullptr )
            {
                std::string context_error_string
                    = "failed to create OpenGL context via SDL2 window: ";
                context_error_string += SDL_GetError();
                SDL_DestroyWindow( sdl_window );
                throw std::runtime_error( context_error_string );
            }
        }
        ~SDL_window()
        {
            SDL_GL_DeleteContext( gl_context );
            SDL_DestroyWindow( sdl_window );
        }
    };
    
    class GL_shader
    {
    public:
        GLuint id;
        
        GL_shader( GLenum shader_type, const std::string& source )
        {
            id = glCreateShader( shader_type );
            
            auto source_c_string = source.c_str();
            glShaderSource( id, 1, &source_c_string, nullptr );
            glCompileShader( id );
            
            GLint status;
            glGetShaderiv( id, GL_COMPILE_STATUS, &status );
            if( status != GL_TRUE )
            {
                char log_buffer[ 1024 ];
                glGetShaderInfoLog(
                    id,
                    1024,
                    NULL,
                    log_buffer
                );
                std::string shader_error_string = (
                    "failed to compile shader:\n"
                    + std::string( log_buffer )
                );
                glDeleteShader( id );
                throw std::runtime_error( shader_error_string );
            }
        }
        
        static GL_shader from_file(
            GLenum shader_type,
            const std::string& filename
        )
        {
            std::filebuf source_file;
            source_file.open( filename, std::ios_base::in );
            if( !source_file.is_open() )
                throw std::runtime_error(
                    "could not open shader source file \""
                    + filename
                    + "\""
                );
            std::string source = std::string(
                std::istreambuf_iterator< char >( &source_file ),
                {}
            );
            try
            {
                return GL_shader( shader_type, source );
            }
            catch( const std::runtime_error& e )
            {
                throw std::runtime_error(
                    "failed to compile shader file \""
                    + filename
                    + "\": "
                    + e.what()
                );
            }
        }
        
        ~GL_shader()
        {
            glDeleteShader( id );
        }
    };
    
    class GL_shader_program
    {
    public:
        class no_such_variable : public std::runtime_error
        {
            using runtime_error::runtime_error;
        };
        class wrong_variable_type : public std::runtime_error
        {
            using runtime_error::runtime_error;
        };
        
        GLuint id;
        GLuint vao_id;
        
        GL_shader_program( const std::vector< GLuint >& shaders )
        {
            glGenVertexArrays( 1, &vao_id );
            glBindVertexArray( vao_id );
            
            id = glCreateProgram();
            for( auto& shader_id : shaders )
                glAttachShader( id, shader_id );
            
            // Note: use glDrawBuffers when rendering to multiple buffers,
            // because only the first output will be enabled by default.
            glBindFragDataLocation( id, 0, "color_out" );
            
            glLinkProgram( id );
        }
        
        ~GL_shader_program()
        {
            glDeleteProgram( id );
            glDeleteVertexArrays( 1, &vao_id );
        }
        
        void use()
        {
            glUseProgram( id );
            glBindVertexArray( vao_id );
        }
        
        GLint attribute( const std::string& attribute_name )
        {
            GLint result = glGetAttribLocation( id, attribute_name.c_str() );
            if( result == -1 )
                throw no_such_variable(
                    "unable to get attribute \""
                    + attribute_name
                    + "\" from shader program "
                    + std::to_string( id )
                    + " (nonexistent or reserved)"
                );
            return result;
        }
        
        GLint uniform( const std::string& uniform_name )
        {
            GLint result = glGetUniformLocation( id, uniform_name.c_str() );
            if( result == -1 )
                throw no_such_variable(
                    "unable to get uniform \""
                    + uniform_name
                    + "\" from shader program "
                    + std::to_string( id )
                    + " (nonexistent or reserved)"
                );
            return result;
        }
        
        template< typename T > void set_uniform(
            const std::string& uniform_name,
            const T& value
        );
        
        // TODO: Make this non-reliant on exceptions?
        template< typename T > bool try_set_uniform(
            const std::string& uniform_name,
            const T& value
        )
        {
            try
            {
                set_uniform( uniform_name, value );
                return true;
            }
            catch( const no_such_variable& e )
            {
                return false;
            }
        }
    };
    
    template<> void GL_shader_program::set_uniform< float >(
        const std::string& uniform_name,
        const float& value
    )
    {
        auto uniform_id = uniform( uniform_name );
        glUniform1f( uniform_id, value );
        if( glGetError() != GL_NO_ERROR )
            throw wrong_variable_type(
                "unable to set float uniform of program "
                + std::to_string( id )
            );
    }
    
    template<> void GL_shader_program::set_uniform< int >(
        const std::string& uniform_name,
        const int& value
    )
    {
        auto uniform_id = uniform( uniform_name );
        glUniform1i( uniform_id, value );
        if( glGetError() != GL_NO_ERROR )
            throw wrong_variable_type(
                "unable to set integer uniform of program "
                + std::to_string( id )
            );
    }
    
    template<> void GL_shader_program::set_uniform< glm::mat4 >(
        const std::string& uniform_name,
        const glm::mat4& value
    )
    {
        auto uniform_id = uniform( uniform_name );
        glUniformMatrix4fv(
            uniform_id,
            1,          // Number of matrices
            GL_FALSE,   // Transpose matrix before use
            glm::value_ptr( value )
        );
        if( glGetError() != GL_NO_ERROR )
            throw wrong_variable_type(
                "unable to set matrix4 uniform of program "
                + std::to_string( id )
            );
    }
    
    bool load_bound_texture( std::string filename )
    {
        SDL_Surface* sdl_surface = IMG_Load(
            filename.c_str()
        );
        if( !sdl_surface )
            throw std::runtime_error(
                "failed to load texture \""
                + filename
                + "\": "
                + IMG_GetError()
            );
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
        
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
        // float texture_border_color[] = { 1.0f, 0.0f, 0.0f };
        // glTexParameterfv(
        //     GL_TEXTURE_2D,
        //     GL_TEXTURE_BORDER_COLOR,
        //     texture_border_color
        // );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glGenerateMipmap( GL_TEXTURE_2D );
        
        return true;
    }
}


namespace
{
    class tutorial_manager
    {
    public:
        GLuint triangle_vbo;
        GLuint triangle_ebo;
        
        // glm::mat4 transform;
        
        tutorial_manager(
            gl_tut::GL_shader_program& shader_program,
            int argc,
            char* argv[]
        )
        {
            // Load textures ///////////////////////////////////////////////////
            
            
            GLuint textures[ 2 ];
            glGenTextures( 2, textures );
            
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, textures[ 0 ] );
            gl_tut::load_bound_texture( "../resources/textures/rgb.png" );
            shader_program.set_uniform( "texture_A", 0 );
            
            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_2D, textures[ 1 ] );
            gl_tut::load_bound_texture( "../resources/textures/b&w.png" );
            shader_program.set_uniform( "texture_B", 1 );
            
            
            // Create shader input data ////////////////////////////////////////
            
            
            float triangle_vertices[] = {
            //  Position ---| Color ----------| Texture --|
            //      X,     Y,    R,    G,    B,    S,    T
                -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //    top left
                 0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //    top right
                 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // bottom right
                -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // bottom left
            };
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
            glGenBuffers( 1, &triangle_ebo );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, triangle_ebo );
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sizeof( triangle_elements ),
                triangle_elements,
                GL_STATIC_DRAW
            );
            
            
            // Configure shader program attributes /////////////////////////////
            
            
            GLint position_attr = shader_program.attribute( "position" );
            glEnableVertexAttribArray( position_attr );
            glVertexAttribPointer(
                position_attr,       // Data source
                2,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                7 * sizeof( float ), // Component stride in bytes (0 = packed)
                NULL                 // Component offset within stride
            );
            
            GLint color_attr = shader_program.attribute( "color_in" );
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
            
            GLint texture_coord_attr = shader_program.attribute(
                "texture_coord_in"
            );
            glEnableVertexAttribArray( texture_coord_attr );
            glVertexAttribPointer(
                texture_coord_attr,  // Data source
                2,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                7 * sizeof( float ), // Component stride in bytes (0 = packed)
                ( void* )( 5 * sizeof( float ) )
                                     // Component offset within stride
            );
        }
        
        void update(
            gl_tut::GL_shader_program& shader_program,
            const std::chrono::high_resolution_clock::time_point start_time,
            const std::chrono::high_resolution_clock::time_point current_time
        )
        {
            // // DEBUG:
            // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            
            glm::mat4 transform_model = glm::mat4( 1.0f );
            // glm::mat4 transform_model = glm::rotate(
            //     glm::mat4( 1.0f ),
            //     glm::radians( std::chrono::duration_cast<
            //         std::chrono::duration< float >
            //     >( current_time - start_time ).count() * 10.0f ),
            //     glm::vec3( 0.0f, 0.0f, 1.0f )
            // );
            shader_program.try_set_uniform(
                "transform_model",
                transform_model
            );
            
            glm::mat4 transform_view = glm::lookAt(
                glm::vec3( 1.2f, 1.2f, 1.2f ),  // Camera position
                glm::vec3( 0.0f, 0.0f, 0.0f ),  // Look-at point
                glm::vec3( 0.0f, 0.0f, 1.0f )   // Up unit vector
            );
            shader_program.try_set_uniform(
                "transform_view",
                transform_view
            );
            
            glm::mat4 transform_projection = glm::perspective(
                glm::radians( 45.0f ),  // Vertical FoV
                800.0f / 600.0f,        // Aspect ratio
                1.0f,                   // Near plane
                10.0f                   // Far plane
            );
            shader_program.try_set_uniform(
                "transform_projection",
                transform_projection
            );
            
            glDrawElements(
                GL_TRIANGLES,       // Type of primitive
                6,                  // Number of elements
                GL_UNSIGNED_INT,    // Type of element
                0                   // Starting at element
            );
        }
        
        ~tutorial_manager()
        {
            glDeleteBuffers( 1, &triangle_vbo );
            glDeleteBuffers( 1, &triangle_ebo );
        }
    };
}


int main( int argc, char* argv[] )
{
    try
    {
        gl_tut::SDL_manager sdl;
        
        gl_tut::SDL_window window(
            "OpenGL",
            SDL_WINDOWPOS_CENTERED, // 100,
            SDL_WINDOWPOS_CENTERED, // 100,
            800,
            600,
            SDL_WINDOW_OPENGL // | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN
        );
        
        // Run GLEW stuff _after_ creating SDL/GL context
    #ifndef __APPLE__
        glewExperimental = GL_TRUE;
        if( glewInit() != GLEW_OK )
        {
            std::cerr << "failed to initialize GLEW" << std::endl;
            return -1;
        }
    #endif
        
        auto vertex_shader = gl_tut::GL_shader::from_file(
            GL_VERTEX_SHADER,
            "../src/vertex_shader.vert"
        );
        auto fragment_shader = gl_tut::GL_shader::from_file(
            GL_FRAGMENT_SHADER,
            "../src/fragment_shader.frag"
        );
        
        gl_tut::GL_shader_program shader_program( {
            vertex_shader.id,
            fragment_shader.id
        } );
        shader_program.use();
        
        auto tutorial = tutorial_manager( shader_program, argc, argv );
        
        glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto previous_time = start_time;
        
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
                    && SDL_GetWindowFlags( window.sdl_window ) & (
                          SDL_WINDOW_FULLSCREEN
                        | SDL_WINDOW_FULLSCREEN_DESKTOP
                    )
                )
                    break;
            }
            
            auto current_time = std::chrono::high_resolution_clock::now();
            
            shader_program.try_set_uniform(
                "time_absolute",
                std::chrono::duration_cast<
                    std::chrono::duration< float >
                >( current_time - start_time ).count()
            );
            shader_program.try_set_uniform(
                "time_delta",
                std::chrono::duration_cast<
                    std::chrono::duration< float >
                >( current_time - previous_time ).count()
            );
            
            glClear(
                  GL_COLOR_BUFFER_BIT
                | GL_DEPTH_BUFFER_BIT
                | GL_STENCIL_BUFFER_BIT
            );
            
            shader_program.use();
            
            tutorial.update(
                shader_program,
                start_time,
                current_time
            );
            
            SDL_GL_SwapWindow( window.sdl_window );
            
            previous_time = current_time;
        }
        
        return 0;
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
