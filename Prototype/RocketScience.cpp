// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include <SDL.h>

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"

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
    Renderer::Camera::Info *camera = nullptr;
    m_cameraHandle = platform.stateDb.createObjectAndRefState(
        Renderer::Camera::Info::STATE, &camera);
    camera->position = glm::fvec4(20.0f, 20.0f, 20.0f, 1.0f);
    camera->target   = glm::fvec4( 0.0f,  0.0f,  0.0f, 1.0f);

    platform.renderer.activeCameraHandle = m_cameraHandle;

    {
        Renderer::Mesh::Info *meshInfo = nullptr;
        m_gridMeshHandle = platform.stateDb.createObjectAndRefState(
            Renderer::Mesh::Info::STATE, &meshInfo);
        meshInfo->modelAsset = platform.assets.asset("Assets/Grid.obj");
    }
    {
        Renderer::Mesh::Info *meshInfo = nullptr;
        m_arrowMeshHandle = platform.stateDb.createObjectAndRefState(
            Renderer::Mesh::Info::STATE, &meshInfo);
        meshInfo->modelAsset = platform.assets.asset("Assets/Arrow.obj");
    }

    const bool enableRocket = true;

    for (int meshIdx = 0; meshIdx < 64; ++meshIdx)
    {
        Renderer::Mesh::Info *mesh = nullptr;
        u64 meshHandle = platform.stateDb.createObjectAndRefState(
            Renderer::Mesh::Info::STATE, &mesh);
        mesh->translation = glm::fvec4(glm::linearRand(
            glm::fvec3(-10.0f, -10.0f, 0.0f), glm::fvec3(+10.0f, +10.0f, +40.0f)), 1.0);

        if (enableRocket && meshIdx == 0)
        {
            mesh->translation = glm::fvec4(0.0f, 0.0f, 10.0f, 1.0);
            mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
            mesh->modelAsset = platform.assets.asset("Assets/Pusher.obj");
        }
        else if (rand() % 9 > 5)
        {
            mesh->modelAsset = platform.assets.asset("Assets/Cube.obj");
        }
        else if (rand() % 9 > 2)
        {
            mesh->modelAsset = platform.assets.asset("Assets/Sphere.obj");
        }
        else
        {
            mesh->modelAsset = platform.assets.asset("Assets/Torus.obj");
        }

        m_meshHandles.push_back(meshHandle);
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(Platform &platform)
{
    platform.stateDb.destroyObject(m_gridMeshHandle);
    platform.stateDb.destroyObject(m_arrowMeshHandle);
    for (auto rigidBodyIter : m_rigidBodyByMeshHandle)
    {
        platform.stateDb.destroyObject(rigidBodyIter.second);
    }
    for (auto meshHandle : m_meshHandles)
    {
        platform.stateDb.destroyObject(meshHandle);
    }
    platform.stateDb.destroyObject(m_cameraHandle);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, double deltaTimeInS)
{
    // FIXME(martinmo): Add keyboard input to platform abstraction (remove dependency to SDL)
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    double horizontalRotationInDeg = 0.0f;
    if (state[SDL_SCANCODE_RIGHT]) horizontalRotationInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_LEFT])  horizontalRotationInDeg -= deltaTimeInS * 90.0;
    double verticalRotationInDeg = 0.0f;
    if (state[SDL_SCANCODE_DOWN]) verticalRotationInDeg += deltaTimeInS * 90.0;
    if (state[SDL_SCANCODE_UP])   verticalRotationInDeg -= deltaTimeInS * 90.0;
    double translationInM = 0.0f;
    if (state[SDL_SCANCODE_W]) translationInM += deltaTimeInS * 20.0;
    if (state[SDL_SCANCODE_S]) translationInM -= deltaTimeInS * 20.0;

    Renderer::Camera::Info *cameraInfo = nullptr;
    platform.stateDb.refState(Renderer::Camera::Info::STATE, m_cameraHandle, &cameraInfo);

    glm::fvec3 cameraDir = (cameraInfo->target - cameraInfo->position).xyz();

    // Vertical rotation axis in world space
    // FIXME(martinmo): Correctly handle near +/- 90 deg cases
    glm::fvec3 verticalRotationAxis = cameraDir;
    std::swap(verticalRotationAxis.x, verticalRotationAxis.y);
    verticalRotationAxis.y = -verticalRotationAxis.y;
    verticalRotationAxis.z = 0.0f;
    verticalRotationAxis = glm::normalize(verticalRotationAxis);

    glm::fmat4 xform;
    xform = glm::translate(xform, cameraInfo->target.xyz());
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
    xform = glm::translate(xform, -cameraInfo->target.xyz());
    cameraInfo->position = xform * cameraInfo->position;
    if (glm::abs(translationInM) > 0.001)
    {
        cameraInfo->position += glm::fvec4(
            glm::normalize(cameraDir) * float(translationInM), 0.0);
    }

    if (state[SDL_SCANCODE_P] && m_rigidBodyByMeshHandle.size() < m_meshHandles.size())
    {
        u64 meshHandle = m_meshHandles[m_rigidBodyByMeshHandle.size()];

        Physics::RigidBody::Info *rigidBodyInfo = nullptr;
        u64 rigidBodyHandle = platform.stateDb.createObjectAndRefState(
            Physics::RigidBody::Info::STATE, &rigidBodyInfo);
        rigidBodyInfo->meshHandle = meshHandle;

        Renderer::Mesh::Info *meshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, meshHandle, &meshInfo);

        if (meshInfo->modelAsset == platform.assets.asset("Assets/Pusher.obj"))
        {
            Physics::Force::Info *forceInfo = nullptr;
            m_pusherForce = platform.stateDb.createObjectAndRefState(
                Physics::Force::Info::STATE, &forceInfo);
            forceInfo->rigidBodyHandle = rigidBodyHandle;
            forceInfo->enabled = 1;
        }
        else if (meshInfo->modelAsset == platform.assets.asset("Assets/Sphere.obj"))
        {
            rigidBodyInfo->collisionShapeType =
                Physics::RigidBody::Info::CollisionShapeType::BOUNDING_SPHERE;
        }

        m_rigidBodyByMeshHandle[meshHandle] = rigidBodyHandle;
    }

    if (m_pusherForce)
    {
        Renderer::Mesh::Info *meshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, m_meshHandles.front(), &meshInfo);

        Physics::Force::Info *forceInfo = nullptr;
        platform.stateDb.refState(Physics::Force::Info::STATE, m_pusherForce, &forceInfo);

        glm::fvec3 euler = glm::degrees(glm::eulerAngles(meshInfo->rotation));
        float &pitch = euler.x;
        float &yaw   = euler.y;

        SDL_Log("Pitch(X): %8.1f Yaw(Z): %8.1f", pitch, yaw);

        /*
        // Top wing position
        forceInfo->position = modelRot * glm::fvec3(0.0f, -2.64, 2.75f);
        // Head position
        forceInfo->position = modelRot * glm::fvec3(0.0f,  3.61, 0.00f);
        */

        // Tail/main engine position
        forceInfo->position = glm::mat3_cast(meshInfo->rotation) * glm::fvec3(0.0f, -2.14, 0.00f);

        /*
        // This force applied to top wing makes rocket spin clockwise
        forceInfo->force = modelRot * glm::fvec3(10.0f, 0.0f, 0.0f);
        */

        // Main engine params
        float mainEngineForce = 0.0f;
        float mainEnginePitch = 0.0f;
        float mainEngineYaw   = 0.0f;

        // Main engine force to keep height of 10 m
        if (meshInfo->translation.z < 10.0)
        {
            mainEngineForce = 70.0f;
        }
        else
        {
            mainEngineForce = 20.5f;
        }

        // Counteract rocket pitch and yaw
        mainEnginePitch = 1.0f * -(90.0f - pitch);
        mainEngineYaw   = 1.0f *  ( 0.0f - yaw);

        // Add some jitter to main engine thrust vector
        mainEnginePitch += glm::linearRand(-2.0f, +2.0f);
        mainEngineYaw   += glm::linearRand(-2.0f, +2.0f);

        // FIXME(martinmo): Yaw axis should be Z axis *not* Y axis
        // FIXME(martinmo): (euler angles/gimbal lock because of 90 deg around X?)

        glm::fquat mainEnginePitchRot =
            glm::angleAxis(glm::radians(mainEnginePitch), glm::fvec3(1.0, 0.0, 0.0));
        glm::fquat mainEngineYawRot =
            glm::angleAxis(glm::radians(mainEngineYaw),   glm::fvec3(0.0, 1.0, 0.0));
        glm::fquat mainEngineRot =
            meshInfo->rotation * mainEnginePitchRot * mainEngineYawRot;

        forceInfo->force = glm::mat3_cast(mainEngineRot) * glm::fvec3(0.0f, mainEngineForce, 0.0f);

        // Use arrow mesh to display thrust vector
        // FIXME(martinmo): The arrow does not really point in thrust direction
        Renderer::Mesh::Info *arrowMeshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, m_arrowMeshHandle, &arrowMeshInfo);
        arrowMeshInfo->translation =
            glm::vec4(forceInfo->position, 0.0f) + meshInfo->translation;
        arrowMeshInfo->rotation =
            glm::fquat(mainEngineRot.w, -mainEngineRot.x, -mainEngineRot.y, -mainEngineRot.z);

        // Emergency motors off...
        if (glm::abs(mainEngineYaw) > 45.0f || glm::abs(mainEnginePitch > 45.0f))
        {
            platform.stateDb.destroyObject(m_pusherForce);
            m_pusherForce = 0;
        }
    }
}
