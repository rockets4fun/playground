// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#include "Common.hpp"

#include <memory>
#include <cstdlib>

#include <SDL.h>

#include "StateDb.hpp"
#include "Assets.hpp"
#include "Platform.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"
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

    // TODO(martinmo): Implement properly resizable window (propagate to renderer)
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

    {
        Renderer renderer;
        Physics physics;
        RocketScience rocketScience;

        StateDb stateDb;
        Assets assets;

        Platform platform(stateDb, assets, renderer);

        std::vector< ModuleIf * > modules = { &renderer, &physics, &rocketScience };
        std::vector< ModuleIf * > reverseModules = modules;
        std::reverse(reverseModules.begin(), reverseModules.end());

        for (auto &module : modules)
        {
            module->registerTypesAndStates(stateDb);
        }
        for (auto &module : modules)
        {
            if (!module->initialize(platform))
            {
                SDL_Log("ERROR: Failed to initialize module");
                // FIXME(martinmo): Shutdown modules already initilized
                return EXIT_FAILURE;
            }
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
            for (auto &module : modules)
            {
                module->update(platform, deltaTimeInS);
            }

            SDL_GL_SwapWindow(window);
        }

        for (auto &module : reverseModules)
        {
            module->shutdown(platform);
        }
    }

    SDL_GL_DeleteContext(context);

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();

    return EXIT_SUCCESS;
}
