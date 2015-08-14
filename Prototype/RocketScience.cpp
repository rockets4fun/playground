// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include <SDL.h>

#include "Math.hpp"

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
    camera->position = glm::fvec3(20.0f, 20.0f, 20.0f);
    camera->target   = glm::fvec3( 0.0f,  0.0f,  0.0f);

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

    m_oceanModelAsset = platform.assets.asset("procedural/ocean",
        Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);

    const bool enableRocket = true;

    for (int meshIdx = 0; meshIdx < 128; ++meshIdx)
    {
        Renderer::Mesh::Info *mesh = nullptr;
        u64 meshHandle = platform.stateDb.createObjectAndRefState(
            Renderer::Mesh::Info::STATE, &mesh);
        mesh->translation = glm::linearRand(
            glm::fvec3(-10.0f, -10.0f, +20.0f), glm::fvec3(+10.0f, +10.0f, +40.0f));

        if (enableRocket && meshIdx == 0)
        {
            mesh->translation = glm::fvec3(0.0f, 0.0f, 10.0f);
            mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
            mesh->modelAsset = platform.assets.asset("Assets/Pusher.obj");
        }
        else if (rand() % 9 > 5)
        {
            mesh->modelAsset = platform.assets.asset("Assets/AppleCube.obj");
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
    for (auto meshHandle : m_oceanMeshHandles)
    {
        platform.stateDb.destroyObject(meshHandle);
    }
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
float oceanEquation(const glm::fvec2 &position, const glm::fvec2 &unitSize, double timeInS)
{
    float twoPi = 2.0f * glm::pi< float >();
    float result = 0.0f;
    result += 0.50f * sinf((twoPi / (0.500f * unitSize.x)) * (position.x + position.y + timeInS));
    result += 0.50f * sinf((twoPi / (1.000f * unitSize.x)) * (position.x - position.y + timeInS));
    result += 0.10f * sinf((twoPi / (0.250f * unitSize.x)) * (position.x + position.y - timeInS * 0.5f));
    return result;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, double deltaTimeInS)
{
    m_timeInS += deltaTimeInS;

    // Update tileable dynamic ocean model
    {
        const int vertexCount = 32;
        const float vertexDist = 1.0;
        const glm::fvec2 unitSize = glm::fvec2((vertexCount - 1) * vertexDist);

        Assets::Model *model = platform.assets.refModel(m_oceanModelAsset);
        if (model->positions.empty())
        {
            // Create ocean tile geometry
            for (int y = 0; y < vertexCount - 1; ++y)
            {
                for (int x = 0; x < vertexCount - 1; ++x)
                {
                    // Clockwise quad starting from bottom left
                    glm::fvec3  bottomLeft((x + 0) * vertexDist, (y + 0) * vertexDist, 0.0f);
                    glm::fvec3     topLeft((x + 0) * vertexDist, (y + 1) * vertexDist, 0.0f);
                    glm::fvec3    topRight((x + 1) * vertexDist, (y + 1) * vertexDist, 0.0f);
                    glm::fvec3 bottomRight((x + 1) * vertexDist, (y + 0) * vertexDist, 0.0f);
                    // Triangles covering quad
                    model->positions.push_back(bottomLeft);
                    model->positions.push_back(topRight);
                    model->positions.push_back(topLeft);
                    model->positions.push_back(bottomLeft);
                    model->positions.push_back(bottomRight);
                    model->positions.push_back(topRight);
                    // Triangle normals
                    glm::fvec3 normal(0.0f, 0.0f, 1.0f);
                    for (int vIdx = 0; vIdx < 6; ++vIdx) model->normals.push_back(normal);
                    // Triangle colors
                    float value = glm::linearRand(0.5f, 1.0f);
                    glm::fvec3 color(value, value, value);
                    for (int vIdx = 0; vIdx < 6; ++vIdx) model->colors.push_back(color);
                }
            }
            // Create ocean tiles
            const int tileCount = 4;
            for (int y = 0; y < tileCount; ++y)
            {
                for (int x = 0; x < tileCount; ++x)
                {
                    Renderer::Mesh::Info *meshInfo = nullptr;
                    u64 oceanMeshHandle = platform.stateDb.createObjectAndRefState(
                        Renderer::Mesh::Info::STATE, &meshInfo);
                    meshInfo->modelAsset = m_oceanModelAsset;
                    meshInfo->translation = glm::fvec3(
                        glm::fvec2(float(x), float(y)) * unitSize -
                        0.5f * float(tileCount) * unitSize, -10.0f);
                    m_oceanMeshHandles.push_back(oceanMeshHandle);
                }
            }
        }
        // Update positions according to ocean equation
        for (glm::fvec3 &position : model->positions)
        {
            // Update Z and normal depending on X and Y
            position.z = oceanEquation(position.xy(), unitSize, m_timeInS);
        }
        // Update normals and colors according to ocean equation
        const int triCount = model->normals.size() / 3;
        for (int triIdx = 0; triIdx < triCount; ++triIdx)
        {
            glm::fvec3 normal = glm::normalize(glm::cross(
                model->positions[triIdx * 3 + 1] - model->positions[triIdx * 3 + 0],
                model->positions[triIdx * 3 + 2] - model->positions[triIdx * 3 + 0]));
            model->normals[triIdx * 3 + 0] = normal;
            model->normals[triIdx * 3 + 1] = normal;
            model->normals[triIdx * 3 + 2] = normal;
        }
    }

    // FIXME(martinmo): Add keyboard input to platform abstraction (remove dependency to SDL)
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    // Update camera
    {
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
        cameraInfo->position = glm::fvec3(xform * glm::fvec4(cameraInfo->position, 1.0f));
        if (glm::abs(translationInM) > 0.001)
        {
            cameraInfo->position += glm::normalize(cameraDir) * float(translationInM);
        }
    }

    // Turn meshes into rigid bodies
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
            rigidBodyInfo->collisionShapeType =
                Physics::RigidBody::Info::CollisionShapeType::CONVEX_HULL_COMPOUND;
            {
                Physics::Force::Info *forceInfo = nullptr;
                m_pusherForceHandle = platform.stateDb.createObjectAndRefState(
                    Physics::Force::Info::STATE, &forceInfo);
                forceInfo->rigidBodyHandle = rigidBodyHandle;
                forceInfo->enabled = 1;
            }
        }
        else if (meshInfo->modelAsset == platform.assets.asset("Assets/Sphere.obj"))
        {
            rigidBodyInfo->collisionShapeType =
                Physics::RigidBody::Info::CollisionShapeType::BOUNDING_SPHERE;
        }

        m_rigidBodyByMeshHandle[meshHandle] = rigidBodyHandle;
    }

    // Update rocket force control logic
    if (m_pusherForceHandle)
    {
        Renderer::Mesh::Info *meshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, m_meshHandles.front(), &meshInfo);

        Physics::Force::Info *forceInfo = nullptr;
        platform.stateDb.refState(Physics::Force::Info::STATE, m_pusherForceHandle, &forceInfo);

        glm::fmat3 meshRot = glm::mat3_cast(meshInfo->rotation);

        /*
        // Top wing position
        forceInfo->position = meshRot * glm::fvec3(0.00f, -2.64, 2.75f);
        // Head position
        forceInfo->position = meshRot * glm::fvec3(0.00f,  3.61, 0.00f);
        */

        // Tail/main engine position
        forceInfo->position = meshRot * glm::fvec3(0.00f, -2.14, 0.00f);

        /*
        // This force applied to top wing makes rocket spin clockwise
        forceInfo->force = modelRot * glm::fvec3(10.0f, 0.0f, 0.0f);
        */

        // Main engine force to keep height of 10 m
        float mainEngineForce = 0.0f;
        if (meshInfo->translation.z < 10.0)
        {
            // Mass of rocket is 5 kg (hard-coded for all RBs ATM)
            // ==> Force needs to be at least 9.81 x 5 ==> 50
            mainEngineForce = 55.0f;
        }
        else
        {
            mainEngineForce = 45.0f;
        }

        // Rocket's local +Y should be aligned to global +Z
        glm::fvec3 nominalDir = glm::fvec3(0.0, 0.0, 1.0);
        glm::fvec3 actualDir  = meshRot * glm::fvec3(0.0f, 1.0f, 0.0f);

        // Tilt nominal direction to steer rocket towards origin
        nominalDir += glm::normalize(-meshInfo->translation) / 20.0f;
        nominalDir  = glm::normalize(nominalDir);

        // Thrust vector tries to align rocket with nominal direction
        glm::fvec3 thrustVector = glm::normalize(nominalDir + 1.2f * (actualDir - nominalDir));

        // Add some main engine jitter
        glm::fvec3 maxNoise = glm::fvec3(0.05f, 0.05f, 0.05f);
        thrustVector += glm::linearRand(-maxNoise, maxNoise);

        forceInfo->force = thrustVector * mainEngineForce;

        // Use arrow mesh to display thrust vector
        Renderer::Mesh::Info *arrowMeshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, m_arrowMeshHandle, &arrowMeshInfo);
        arrowMeshInfo->translation = meshInfo->translation + forceInfo->position;
        arrowMeshInfo->rotation = Math::rotateFromTo(
            glm::fvec3(0.0f, 1.0f, 0.0f), -forceInfo->force);

        // Emergency motors off...
        double errorAngleInDeg = glm::degrees(
            glm::angle(Math::rotateFromTo(nominalDir, actualDir)));
        //SDL_Log("errorAngleInDeg: %5.2f", errorAngleInDeg);
        if (glm::abs(errorAngleInDeg) > 20.0f)
        {
            platform.stateDb.destroyObject(m_pusherForceHandle);
            m_pusherForceHandle = 0;
        }
    }
}
