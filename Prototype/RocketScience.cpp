// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <glm/gtc/matrix_transform.hpp>

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
    Renderer::CameraInfo *cameraInfo = (Renderer::CameraInfo *)
        platform.stateDb.state(platform.RendererCameraInfo, m_camera);
    cameraInfo->position = glm::fvec4(1.0f, 1.0f, 0.75f, 1.0f);
    cameraInfo->target   = glm::fvec4(0.0f, 0.0f, 0.0f, 1.0f);
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
    double rotateHorizontalInDeg = 0.0f;
    double rotateVerticalInDeg = 0.0f;

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_RIGHT]) rotateHorizontalInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_LEFT])  rotateHorizontalInDeg -= deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_UP])    rotateVerticalInDeg   += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_DOWN])  rotateVerticalInDeg   -= deltaTimeInS * 90.0;

    Renderer::CameraInfo *cameraInfo = (Renderer::CameraInfo *)
        platform.stateDb.state(platform.RendererCameraInfo, m_camera);

    glm::fmat4 xform;
    xform = glm::translate(xform, cameraInfo->target.xyz());
    if (glm::abs(rotateHorizontalInDeg) > 0.1)
    {
        xform = glm::rotate(xform, glm::radians(float(rotateHorizontalInDeg)), glm::fvec3(0.0f, 0.0f, 1.0f));
    }
    if (glm::abs(rotateVerticalInDeg) > 0.1)
    {
        xform = glm::rotate(xform, glm::radians(float(rotateVerticalInDeg)),   glm::fvec3(1.0f, 0.0f, 0.0f));
    }
    xform = glm::translate(xform, -cameraInfo->target.xyz());
    cameraInfo->position = xform * cameraInfo->position;
}
