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
    double horizontalRotationInDeg = 0.0f;
    double verticalRotationInDeg = 0.0f;

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_RIGHT]) horizontalRotationInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_LEFT])  horizontalRotationInDeg -= deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_DOWN])  verticalRotationInDeg   += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_UP])    verticalRotationInDeg   -= deltaTimeInS * 90.0;

    Renderer::CameraInfo *camera = (Renderer::CameraInfo *)
        platform.stateDb.state(platform.RendererCameraInfo, m_camera);

    // Vertical rotation axis in world space
    glm::fvec3 verticalRotationAxis = (camera->target - camera->position).xyz();
    std::swap(verticalRotationAxis.x, verticalRotationAxis.y);
    verticalRotationAxis.y = -verticalRotationAxis.y;
    verticalRotationAxis.z = 0.0f;
    verticalRotationAxis = glm::normalize(verticalRotationAxis);

    glm::fmat4 xform;
    xform = glm::translate(xform, camera->target.xyz());
    if (glm::abs(horizontalRotationInDeg) > 0.1)
    {
        xform = glm::rotate(xform, glm::radians(float(horizontalRotationInDeg)), glm::fvec3(0.0f, 0.0f, 1.0f));
    }
    if (glm::abs(verticalRotationInDeg) > 0.1)
    {
        xform = glm::rotate(xform, glm::radians(float(verticalRotationInDeg)), verticalRotationAxis);
    }
    xform = glm::translate(xform, -camera->target.xyz());
    camera->position = xform * camera->position;
}
