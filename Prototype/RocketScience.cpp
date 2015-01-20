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

    Renderer::Mesh::Info *gridMeshInfo = nullptr;
    m_gridMeshHandle = platform.stateDb.createObjectAndRefState(
        Renderer::Mesh::Info::STATE, &gridMeshInfo);
    gridMeshInfo->modelAsset = platform.assets.asset("Assets/Grid.obj");

    for (int meshIdx = 0; meshIdx < 4; ++meshIdx)
    {
        Renderer::Mesh::Info *mesh = nullptr;
        u64 meshHandle = platform.stateDb.createObjectAndRefState(
            Renderer::Mesh::Info::STATE, &mesh);
        mesh->translation = glm::fvec4(glm::linearRand(
            glm::fvec3(-10.0f, -10.0f, 0.0f), glm::fvec3(+10.0f, +10.0f, +40.0f)), 1.0);

        if (meshIdx == 0)
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
        u64 meshHandle = 0;
        do
        {
            meshHandle = m_meshHandles[rand() % m_meshHandles.size()];
        }
        while (m_rigidBodyByMeshHandle[meshHandle]);

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
            forceInfo->force = glm::fvec3(0.0f, 0.0f, 45.0f);
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

        glm::fmat4 modelToWorld = glm::mat4_cast(meshInfo->rotation);

        Physics::Force::Info *forceInfo = nullptr;
        platform.stateDb.refState(Physics::Force::Info::STATE, m_pusherForce, &forceInfo);

        // Top wing
        forceInfo->position = (modelToWorld * glm::fvec4(0.0f, -2.64, 2.75f, 1.0f)).xyz();
        /*
        // Head
        forceInfo->position = (modelToWorld * glm::fvec4(0.0f,  3.61, 0.00f, 1.0f)).xyz();
        // Tail
        forceInfo->position = (modelToWorld * glm::fvec4(0.0f, -2.14, 0.00f, 1.0f)).xyz();
        */
    }
}
