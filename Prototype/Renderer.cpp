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

// Declared here because we do not want to expose OpenGL implementation details
struct Renderer::GlFuncs
{
    // Buffer clearing functions
    PFNGLCLEARPROC      glClear = nullptr;
    PFNGLCLEARCOLORPROC glClearColor = nullptr;
};

// Declared here because we do not want to expose OpenGL implementation details
struct Renderer::GlState
{
    GLuint dummyVertexShader = 0;
    GLuint dummyFragmentShader = 0;
};

// -------------------------------------------------------------------------------------------------
Renderer::Renderer()
{
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

    // TODO(MARTINMO): Hard-coded vertex and fragment shaders

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::shutdown()
{
}

// -------------------------------------------------------------------------------------------------
void Renderer::update(Platform &platform, real64 deltaTimeInS)
{
    glFuncs->glClearColor(float(rand() % 11) / 10.0f, 0.0, 0.0, 1.0);
    glFuncs->glClear(GL_COLOR_BUFFER_BIT);
}

// -------------------------------------------------------------------------------------------------
#define RENDERER_FUNC(Name) \
    if (!(Name = (decltype(Name))(SDL_GL_GetProcAddress(#Name)))) return false;
bool Renderer::initializeGl()
{
    glFuncs = std::shared_ptr< GlFuncs >(new GlFuncs);
    RENDERER_FUNC(glFuncs->glClear);
    RENDERER_FUNC(glFuncs->glClearColor);

    glState = std::shared_ptr< GlState >(new GlState);

    return true;
}
#undef RENDERER_FUNC
