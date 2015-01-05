// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#include "RocketScience.hpp"

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Renderer.hpp"

#include <SDL.h>

// -------------------------------------------------------------------------------------------------
RocketScience::RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
RocketScience::~RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(Platform &platform)
{
    m_camera = platform.stateDb.createObject(platform.RendererCamera);
    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(Platform &platform)
{
    platform.stateDb.deleteObject(platform.RendererCamera, m_camera);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, real64 deltaTimeInS)
{
    Renderer::CameraInfo *cameraInfo = (Renderer::CameraInfo *)
        platform.stateDb.state(platform.RendererCameraInfo, m_camera);

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_LEFT])  cameraInfo->target.x += deltaTimeInS;
    if (state[SDL_SCANCODE_RIGHT]) cameraInfo->target.x -= deltaTimeInS;
}
