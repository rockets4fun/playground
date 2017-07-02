// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#include "Common.hpp"

#include <memory>
#include <cstdlib>

#include <SDL.h>

#include "Logger.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"
#include "ImGuiEval.hpp"

#include "AppShipLanding.hpp"
#include "AppSpaceThrusters.hpp"

#include "Profiler.hpp"

// -------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Logger logging;

    if (SDL_Init(SDL_INIT_VIDEO))
    {
        Logger::debug("ERROR: Failed to initialize SDL");
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
        "Rocket Science Playground",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 450, SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    int majorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
    int minorVersion = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);
    int profileMask = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);

    Logger::debug("OpenGL context: %d.%d (%s)", majorVersion, minorVersion,
        profileMask == SDL_GL_CONTEXT_PROFILE_CORE ? "core" : "non-core");

    {
        Physics physics;
        ImGuiEval imGuiEval;
        Renderer renderer;

        //AppShipLanding app(physics);
        AppSpaceThrusters app(physics, imGuiEval);

        StateDb sdb;
        Assets assets;

        std::vector< ModuleIf * > modules =
        {
            &physics, &app, &imGuiEval, &renderer
        };

        std::vector< ModuleIf * > modulesReversed = modules;
        std::reverse(modulesReversed.begin(), modulesReversed.end());

        for (auto &module : modules)
        {
            module->registerTypesAndStates(sdb);
        }
        for (auto &module : modules)
        {
            if (!module->initialize(sdb, assets))
            {
                Logger::debug("ERROR: Failed to initialize module");
                // FIXME(martinmo): Shutdown modules already initilized
                return EXIT_FAILURE;
            }
        }

        bool running = true;
        SDL_Event event;
        while (running)
        {
            PROFILER_THREAD(Main, glm::fvec3(0.5f, 0.5f, 0.5f))

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
                module->update(sdb, assets, renderer, deltaTimeInS);
            }

            {
                PROFILER_SECTION(ReloadAssets, glm::fvec3(1.0f, 0.0f, 0.5f));
                assets.reloadModifiedAssets();
            }

            SDL_GL_SwapWindow(window);
        }

        for (auto &module : modulesReversed)
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
