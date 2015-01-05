// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#include "Common.hpp"

#include <memory>
#include <cstdlib>

#include <SDL.h>

#include "StateDb.hpp"
#include "Platform.hpp"
#include "Renderer.hpp"
#include "RocketScience.hpp"

// -------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("ERROR: Failed to initialize SDL");
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // TODO(MARTINMO): Implement properly resizable window (propagate to renderer)
    SDL_Window *window = SDL_CreateWindow("RocketScience Prototype",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480, SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int majorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
    int minorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);
    int profileMask = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);

    SDL_Log("OpenGL context: %d.%d (%s)", majorVersion, minorVersion,
        profileMask == SDL_GL_CONTEXT_PROFILE_CORE ? "core" : "non-core");

    StateDb stateDb;
    Platform platform(stateDb);
    Renderer renderer;
    RocketScience rocketScience;

    if (!renderer.initialize(platform))
    {
        SDL_Log("ERROR: Failed to initialize renderer module");
        return EXIT_FAILURE;
    }
    if (!rocketScience.initialize(platform))
    {
        SDL_Log("ERROR: Failed to initialize rocket science module");
        // FIXME(MARTINMO): This return will not implicitly call shutdown of renderer module
        // FIXME(MARTINMO): --> Should be care about this? (internet says: rather no...)
        return EXIT_FAILURE;
    }

    bool running = true;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        double deltaTimeInS = 1.0 / 60.0;
        rocketScience.update(platform, deltaTimeInS);
        renderer.update(platform, deltaTimeInS);

        SDL_GL_SwapWindow(window);
    }

    rocketScience.shutdown(platform);
    renderer.shutdown(platform);

    SDL_GL_DeleteContext(context);

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();

    return EXIT_SUCCESS;
}
