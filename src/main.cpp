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
            
            // // Note: use glDrawBuffers when rendering to multiple buffers,
            // // because only the first output will be enabled by default.
            // glBindFragDataLocation( id, 0, "color_out" );
            
            const GLchar* feedback_varyings[] = { "value_out" };
            glTransformFeedbackVaryings(
                id,
                1,                      // Number of varyings
                feedback_varyings,      // Varying outputs array
                GL_INTERLEAVED_ATTRIBS  // How data should be written (vs. GL_SEPARATE_ATTRIBS)
            );
            
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
                "unable to set float uniform \""
                + uniform_name
                + "\" of program "
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
                "unable to set integer uniform \""
                + uniform_name
                + "\" of program "
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
                "unable to set matrix4 uniform \""
                + uniform_name
                + "\" of program "
                + std::to_string( id )
            );
    }
    
    template<> void GL_shader_program::set_uniform< glm::vec3 >(
        const std::string& uniform_name,
        const glm::vec3& value
    )
    {
        auto uniform_id = uniform( uniform_name );
        glUniform3f(
            uniform_id,
            value[ 0 ],
            value[ 1 ],
            value[ 2 ]
        );
        if( glGetError() != GL_NO_ERROR )
            throw wrong_variable_type(
                "unable to set vector3 uniform \""
                + uniform_name
                + "\" of program "
                + std::to_string( id )
            );
    }
    
    class GL_framebuffer
    {
    public:
        GLuint id;
        GLuint color_buffer;
        GLuint depth_stencil_buffer;
        
        GL_framebuffer(
            GLsizei width,
            GLsizei height
        )
        {
            glGenFramebuffers( 1, &id );
            glBindFramebuffer( GL_FRAMEBUFFER, id );
            
            glGenTextures( 1, &color_buffer );
            glBindTexture( GL_TEXTURE_2D, color_buffer );
            
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGB,
                width, height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                nullptr
            );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,   // Which attachment
                GL_TEXTURE_2D,
                color_buffer,
                0                       // Mipmap level (not useful)
            );
            
            glGenRenderbuffers( 1, &depth_stencil_buffer );
            glBindRenderbuffer( GL_RENDERBUFFER, depth_stencil_buffer );
            
            glRenderbufferStorage(
                GL_RENDERBUFFER,
                GL_DEPTH24_STENCIL8,
                width, height
            );
            
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_RENDERBUFFER,
                depth_stencil_buffer
            );
            
            if(
                glCheckFramebufferStatus( GL_FRAMEBUFFER )
                != GL_FRAMEBUFFER_COMPLETE
            )
            {
                glDeleteFramebuffers( 1, &id );
                glDeleteTextures( 1, &color_buffer );
                glDeleteRenderbuffers( 1, &depth_stencil_buffer );
                throw std::runtime_error( "failed to complete famebuffer" );
            }
        }
        
        ~GL_framebuffer()
        {
            glDeleteFramebuffers( 1, &id );
            glDeleteTextures( 1, &color_buffer );
            glDeleteRenderbuffers( 1, &depth_stencil_buffer );
        }
    };
    
    class render_step
    {
    public:
        virtual ~render_step() {};
        virtual void run( GL_framebuffer& ) = 0;
    };
}


namespace
{
    const int window_width  = 800;
    const int window_height = 600;
    
    class feedback_render_step : public gl_tut::render_step
    {
    public:
        gl_tut::GL_shader_program* shader_program;
        GLuint data_vbo;
        GLuint result_vbo;
        GLuint query;
        
        feedback_render_step()
        {
            auto shader = gl_tut::GL_shader::from_file(
                GL_VERTEX_SHADER,
                "../src/feedback.vert"
            );
            
            shader_program = new gl_tut::GL_shader_program( {
                shader.id
            } );
            shader_program -> use();
            
            float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
            glGenBuffers( 1, &data_vbo );
            glBindBuffer( GL_ARRAY_BUFFER, data_vbo );
            glBufferData(
               GL_ARRAY_BUFFER,
               sizeof( data ),
               data,
               GL_STATIC_DRAW
            );
            
            GLint data_attr = shader_program -> attribute( "value_in" );
            glEnableVertexAttribArray( data_attr );
            glVertexAttribPointer(
                data_attr,           // Data source
                1,                   // Components per element
                GL_FLOAT,            // Component type
                GL_FALSE,            // Components should be normalized
                1 * sizeof( float ), // Component stride in bytes (0 = packed)
                ( void* )( 0 * sizeof( float ) )
                                     // Component offset within stride
            );
            
            glGenBuffers( 1, &result_vbo );
            glBindBuffer( GL_ARRAY_BUFFER, result_vbo );
            glBufferData(
               GL_ARRAY_BUFFER,
               sizeof( data ),
               nullptr,
               GL_STATIC_READ
            );
            
            glGenQueries( 1, &query );
        }
        
        ~feedback_render_step()
        {
            glDeleteQueries( 1, &query );
            glDeleteBuffers( 1, &result_vbo );
            glDeleteBuffers( 1, &data_vbo );
            delete shader_program;
        }
        
        void run( gl_tut::GL_framebuffer& previous_framebuffer )
        {
            glEnable( GL_RASTERIZER_DISCARD );
            
            glBindBufferBase(
                GL_TRANSFORM_FEEDBACK_BUFFER,
                0,          // Index of output variable
                result_vbo  // Buffer object to bind
            );
            
            glBeginQuery(
                GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
                    // GL_PRIMITIVES_GENERATED, GL_TIME_ELAPSED
                query
            );
            // Must match output of geom shader when using one
            glBeginTransformFeedback( GL_POINTS );
            glDrawArrays( GL_POINTS, 0, 5 );
            glEndTransformFeedback();
            glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );
            
            glFlush();
            
            GLuint count_primitives;
            glGetQueryObjectuiv(
                query,
                GL_QUERY_RESULT,
                &count_primitives
            );
            
            float* feedback = new float[ count_primitives ];
            glGetBufferSubData(
                GL_TRANSFORM_FEEDBACK_BUFFER,
                0,
                (
                    count_primitives    // Number primitives generated
                    * 1                 // Number of floats per primitive
                    * sizeof( float )
                ),
                feedback
            );
            std::cout
                << "got "
                << count_primitives
                << " results:"
                << std::endl
            ;
            for( int i = 0; i < count_primitives; ++i )
                std::cout
                    << "  "
                    << feedback[ i ]
                    << std::endl
                ;
            delete[] feedback;
            throw std::runtime_error( "done" );
            
            glDisable( GL_RASTERIZER_DISCARD );
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
            window_width,
            window_height,
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
        
        std::vector< gl_tut::render_step* > render_steps = {
            new feedback_render_step()
        };
        
        gl_tut::GL_framebuffer preprocessing_framebuffer(
            window_width,
            window_height
        );
        
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
            
            auto step_iter = render_steps.begin();
            while( step_iter != render_steps.end() )
            {
                auto step = *step_iter;
                
                if( ++step_iter == render_steps.end() )
                    // Bind default framebuffer
                    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
                else
                    glBindFramebuffer(
                        GL_FRAMEBUFFER,
                        preprocessing_framebuffer.id
                    );
                
                step -> run( preprocessing_framebuffer );
            }
            
            SDL_GL_SwapWindow( window.sdl_window );
        }
        
        for( auto step : render_steps )
            delete step;
        
        return 0;
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
