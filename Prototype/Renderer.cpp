// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "Renderer.hpp"

// TODO(MARTINMO): Remove dependency to SDL through OpenGL function retrieval interface
// TODO(MARTINMO): This can then be implemented in the platform layer through e.g. SDL/Qt
#include <SDL.h>

#include "Platform.hpp"
#include "StateDb.hpp"

// Include file containing all OpenGL declarations and definitions
// --> This will never be included outside of this class
#include "CoreGl/glcorearb.h"

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlFuncs
{
    // Buffer clearing functions
    PFNGLCLEARCOLORPROC glClearColor = nullptr;
    PFNGLCLEARPROC      glClear = nullptr;
    // Shader object functions
    PFNGLCREATESHADERPROC     glCreateShader = nullptr;
    PFNGLSHADERSOURCEPROC     glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC    glCompileShader = nullptr;
    PFNGLDELETESHADERPROC     glDeleteShader = nullptr;
    PFNGLGETSHADERIVPROC      glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
    // Program object functions
    PFNGLCREATEPROGRAMPROC     glCreateProgram = nullptr;
    PFNGLATTACHSHADERPROC      glAttachShader = nullptr;
    PFNGLDETACHSHADERPROC      glDetachShader = nullptr;
    PFNGLLINKPROGRAMPROC       glLinkProgram = nullptr;
    PFNGLUSEPROGRAMPROC        glUseProgram = nullptr;
    PFNGLDELETEPROGRAMPROC     glDeleteProgram = nullptr;
    PFNGLGETPROGRAMIVPROC      glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlState
{
    GLuint defVertexShader = 0;
    GLuint defFragmentShader = 0;
    GLuint defProgram = 0;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlHelpers
{
    typedef void (APIENTRYP GetProc)
        (GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRYP InfoLogProc)
        (GLuint object, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

    GlHelpers(GlFuncs *funcs);

    bool createAndCompileShader(GLuint &shader, GLenum type, const std::string &source);
    bool createAndLinkSimpleProgram(GLuint &program, GLuint vertexShader, GLuint fragmentShader);
    void printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc);

private:
    GlFuncs *funcs = nullptr;
};

// -------------------------------------------------------------------------------------------------
Renderer::Renderer()
{
    funcs = std::shared_ptr< GlFuncs >(new GlFuncs);
    state = std::shared_ptr< GlState >(new GlState);
    helpers = std::shared_ptr< GlHelpers >(new GlHelpers(funcs.get()));
}

// -------------------------------------------------------------------------------------------------
Renderer::~Renderer()
{
}

// -------------------------------------------------------------------------------------------------
bool Renderer::initialize(Platform &platform)
{
    if (!initializeGl())
    {
        return false;
    }

    std::string vertexShaderSrc =
        "attribute vec4 Position;\n"
        "\n"
        "uniform mat4 ModelViewMatrix;\n"
        "uniform mat4 ProjectionMatrix;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(Position.xyz, 1);\n"
        "}\n";
    helpers->createAndCompileShader(state->defVertexShader, GL_VERTEX_SHADER, vertexShaderSrc);

    std::string fragmentShaderSrc =
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "}\n";
    helpers->createAndCompileShader(
        state->defFragmentShader, GL_FRAGMENT_SHADER, fragmentShaderSrc);

    helpers->createAndLinkSimpleProgram(
        state->defProgram, state->defVertexShader, state->defFragmentShader);
    funcs->glUseProgram(state->defProgram);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::shutdown()
{
}

// -------------------------------------------------------------------------------------------------
void Renderer::update(Platform &platform, real64 deltaTimeInS)
{
    funcs->glClearColor(float(rand() % 11) / 10.0f, 0.0, 0.0, 1.0);
    funcs->glClear(GL_COLOR_BUFFER_BIT);
}

// -------------------------------------------------------------------------------------------------
#define RENDERER_GL_FUNC(Name) \
        { \
            funcs->Name = (decltype(funcs->Name))(SDL_GL_GetProcAddress(#Name)); \
            if (funcs->Name == nullptr) \
            { \
                SDL_Log("ERROR: Failed to resolve '%s'", #Name); \
                return false; \
            } \
        }
bool Renderer::initializeGl()
{
    RENDERER_GL_FUNC(glClearColor);
    RENDERER_GL_FUNC(glClear);

    RENDERER_GL_FUNC(glCreateShader);
    RENDERER_GL_FUNC(glShaderSource);
    RENDERER_GL_FUNC(glCompileShader);
    RENDERER_GL_FUNC(glDeleteShader);
    RENDERER_GL_FUNC(glGetShaderiv);
    RENDERER_GL_FUNC(glGetShaderInfoLog);

    RENDERER_GL_FUNC(glCreateProgram);
    RENDERER_GL_FUNC(glAttachShader);
    RENDERER_GL_FUNC(glDetachShader);
    RENDERER_GL_FUNC(glLinkProgram);
    RENDERER_GL_FUNC(glUseProgram);
    RENDERER_GL_FUNC(glDeleteProgram);
    RENDERER_GL_FUNC(glGetProgramiv);
    RENDERER_GL_FUNC(glGetProgramInfoLog);

    return true;
}
#undef RENDERER_GL_FUNC

// -------------------------------------------------------------------------------------------------
Renderer::GlHelpers::GlHelpers(GlFuncs *funcs) : funcs(funcs)
{
}

// -------------------------------------------------------------------------------------------------
bool Renderer::GlHelpers::createAndCompileShader(GLuint &shader, GLenum type, const std::string &src)
{
    shader = funcs->glCreateShader(type);
    if (!shader)
    {
        SDL_Log("ERROR: Failed to create shader object");
        return false;
    }

    GLint shaderLength = GLint(src.length());
    const GLchar *srcPtr = (const GLchar *)src.c_str();
    funcs->glShaderSource(shader, 1, &srcPtr, &shaderLength);
    funcs->glCompileShader(shader);

    printInfoLog(shader, funcs->glGetShaderiv, funcs->glGetShaderInfoLog);

    GLint compileStatus = GL_FALSE;
    funcs->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        SDL_Log("ERROR: Failed to compile shader");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Renderer::GlHelpers::createAndLinkSimpleProgram(
    GLuint &program, GLuint vertexShader, GLuint fragmentShader)
{
    program = funcs->glCreateProgram();
    if (!program)
    {
        SDL_Log("ERROR: Failed to create program object");
        return false;
    }

    funcs->glAttachShader(program, vertexShader);
    funcs->glAttachShader(program, fragmentShader);
    funcs->glLinkProgram(program);

    printInfoLog(program, funcs->glGetProgramiv, funcs->glGetProgramInfoLog);

    GLint linkStatus = GL_FALSE;
    funcs->glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        SDL_Log("ERROR: Failed to link program");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::GlHelpers::printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc)
{
    // TODO(MARTINMO): Refactor into 'getInfoLog' and separate print
    GLint infoLogLength = 0;
    getProc(object, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 1)
    {
        std::string infoLog(infoLogLength, 'x');
        infoLogProc(object, GLsizei(infoLog.length()), nullptr, &infoLog[0]);
        SDL_Log("----------------------------------------");
        SDL_Log("%s", infoLog.c_str());
        SDL_Log("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
}
