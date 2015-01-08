// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

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
void RocketScience::registerTypesAndStates(StateDb &stateDb)
{
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(Platform &platform)
{
    m_cameraHandle = platform.stateDb.createObject(Renderer::Camera::TYPE);
    Renderer::Camera::Info *camera = (Renderer::Camera::Info *)
        platform.stateDb.state(Renderer::Camera::Info::STATE, m_cameraHandle);
    camera->position = glm::fvec4(20.0f, 20.0f, 20.0f, 1.0f);
    camera->target   = glm::fvec4( 0.0f,  0.0f,  0.0f, 1.0f);

    platform.renderer.activeCameraHandle = m_cameraHandle;

    for (int cubeIdx = 0; cubeIdx < 128; ++cubeIdx)
    {
        u64 meshHandle = platform.stateDb.createObject(Renderer::Mesh::TYPE);
        Renderer::Mesh::Info *mesh = (Renderer::Mesh::Info *)
            platform.stateDb.state(Renderer::Mesh::Info::STATE, meshHandle);
        mesh->position = glm::fvec4(glm::clamp(float(cubeIdx), 0.0f, 1.0f) * glm::linearRand(
            glm::fvec3(-20.0f, -20.0f, 0.0f), glm::fvec3(+20.0f, +20.0f, +40.0f)), 1.0);
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(Platform &platform)
{
    // FIXME(MARTINMO): Delete/destroy all created meshes from 'vector< u64 >'

    platform.stateDb.destroyObject(m_cameraHandle);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, double deltaTimeInS)
{
    // FIXME(MARTINMO): Add keyboard input to platform abstraction (remove dependency to SDL)
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    double horizontalRotationInDeg = 0.0f;
    if (state[SDL_SCANCODE_RIGHT]) horizontalRotationInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_LEFT])  horizontalRotationInDeg -= deltaTimeInS * 90.0;
    double verticalRotationInDeg = 0.0f;
    if (state[SDL_SCANCODE_DOWN]) verticalRotationInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_UP])   verticalRotationInDeg -= deltaTimeInS * 90.0;
    double translationInM = 0.0f;
    if (state[SDL_SCANCODE_W]) translationInM += deltaTimeInS * 10.0;
    if (state[SDL_SCANCODE_S]) translationInM -= deltaTimeInS * 10.0;

    Renderer::Camera::Info *camera = (Renderer::Camera::Info *)
        platform.stateDb.state(Renderer::Camera::Info::STATE, m_cameraHandle);

    glm::fvec3 cameraDir = (camera->target - camera->position).xyz();

    // Vertical rotation axis in world space
    // FIXME(MARTINMO): Correctly handle near +/- 90 deg
    glm::fvec3 verticalRotationAxis = cameraDir;
    std::swap(verticalRotationAxis.x, verticalRotationAxis.y);
    verticalRotationAxis.y = -verticalRotationAxis.y;
    verticalRotationAxis.z = 0.0f;
    verticalRotationAxis = glm::normalize(verticalRotationAxis);

    glm::fmat4 xform;
    xform = glm::translate(xform, camera->target.xyz());
    if (glm::abs(horizontalRotationInDeg) > 0.001)
    {
        xform = glm::rotate(xform, glm::radians(
            float(horizontalRotationInDeg)), glm::fvec3(0.0f, 0.0f, 1.0f));
    }
    if (glm::abs(verticalRotationInDeg) > 0.001)
    {
        xform = glm::rotate(xform, glm::radians(
            float(verticalRotationInDeg)), verticalRotationAxis);
    }
    xform = glm::translate(xform, -camera->target.xyz());
    camera->position = xform * camera->position;
    if (glm::abs(translationInM) > 0.001)
    {
        camera->position += glm::fvec4(
            glm::normalize(cameraDir) * float(translationInM), 0.0);
    }
}
