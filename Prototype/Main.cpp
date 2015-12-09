// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#include "Common.hpp"

#include <memory>
#include <cstdlib>

#include <SDL.h>

#include "Logging.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"
#include "RocketScience.hpp"

#include "Profiling.hpp"

// -------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Logging logging;

    if (SDL_Init(SDL_INIT_VIDEO))
    {
        Logging::debug("ERROR: Failed to initialize SDL");
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

#ifdef COMMON_DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // TODO(martinmo): Implement properly resizable window (propagate to renderer)
    SDL_Window *window = SDL_CreateWindow(
        "RocketScience Prototype", 16, 16, 800, 450, SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int majorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
    int minorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);
    int profileMask = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);

    Logging::debug("OpenGL context: %d.%d (%s)", majorVersion, minorVersion,
        profileMask == SDL_GL_CONTEXT_PROFILE_CORE ? "core" : "non-core");

    {
        Renderer renderer;
        Physics physics;
        RocketScience rocketScience;

        StateDb sdb;
        Assets assets;

        std::vector< ModuleIf * > modulesInit   = { &physics, &renderer, &rocketScience };
        std::vector< ModuleIf * > modulesUpdate = { &physics, &rocketScience, &renderer };

        std::vector< ModuleIf * > modulesInitReversed = modulesInit;
        std::reverse(modulesInitReversed.begin(), modulesInitReversed.end());

        for (auto &module : modulesInit)
        {
            module->registerTypesAndStates(sdb);
        }
        for (auto &module : modulesInit)
        {
            if (!module->initialize(sdb, assets))
            {
                Logging::debug("ERROR: Failed to initialize module");
                // FIXME(martinmo): Shutdown modules already initilized
                return EXIT_FAILURE;
            }
        }

        bool running = true;
        SDL_Event event;
        while (running)
        {
            PROFILING_THREAD(Main, glm::fvec3(0.5f, 0.5f, 0.5f))

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = false;
                }
            }

            double deltaTimeInS = 1.0 / 60.0;
            for (auto &module : modulesUpdate)
            {
                module->update(sdb, assets, renderer, deltaTimeInS);
            }

            {
                PROFILING_SECTION(ReleadAssets, glm::fvec3(1.0f, 0.0f, 0.5f));
                assets.reloadModifiedAssets();
            }

            SDL_GL_SwapWindow(window);
        }

        for (auto &module : modulesInitReversed)
        {
            module->shutdown(sdb);
        }
    }

    SDL_GL_DeleteContext(context);

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();

    return EXIT_SUCCESS;
}
