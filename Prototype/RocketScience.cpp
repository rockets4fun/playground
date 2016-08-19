// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>

#include <glm/gtx/fast_trigonometry.hpp>

#include "Math.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"
#include "Str.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"

const int RocketScience::OCEAN_TILE_VERTEX_COUNT = 32;
const glm::fvec2 RocketScience::OCEAN_TILE_UNIT_SIZE =
    glm::fvec2((OCEAN_TILE_VERTEX_COUNT - 1) * OCEAN_TILE_VERTEX_DIST);
const float RocketScience::OCEAN_TILE_VERTEX_DIST = 1.0f;

const int RocketScience::PLATFORM_SPHERE_COUNT = 4;

// -------------------------------------------------------------------------------------------------
u64 RocketScience::Particle::TYPE = 0;
u64 RocketScience::Particle::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
u64 RocketScience::Thruster::TYPE = 0;
u64 RocketScience::Thruster::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
RocketScience::RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
RocketScience::~RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
void RocketScience::registerTypesAndStates(StateDb &sdb)
{
    Particle::TYPE = sdb.registerType("Particle", 512);
    Particle::Info::STATE = sdb.registerState(Particle::TYPE, "Info", sizeof(Particle::Info));

    Thruster::TYPE = sdb.registerType("Thruster", 64);
    Thruster::Info::STATE = sdb.registerState(Thruster::TYPE, "Info", sizeof(Thruster::Info));
}

// -------------------------------------------------------------------------------------------------
void pushRect2d(Assets::Model *model,
        const glm::fvec2 &ll, const glm::fvec2 &ur,
        const glm::fvec3 &color = glm::fvec3(1.0f, 1.0f, 1.0f), float z = 0.0f)
{
    // World coordinate system: +X=E, +Y=N and +Z=up (front facing is CCW towards negative axis)
    // Lower left triangle
    model->positions.push_back(glm::fvec3(ll.x, ll.y, z)); model->diffuse.push_back(color);
    model->positions.push_back(glm::fvec3(ur.x, ur.y, z)); model->diffuse.push_back(color);
    model->positions.push_back(glm::fvec3(ll.x, ur.y, z)); model->diffuse.push_back(color);
    // Upper right triangle
    model->positions.push_back(glm::fvec3(ur.x, ur.y, z)); model->diffuse.push_back(color);
    model->positions.push_back(glm::fvec3(ll.x, ll.y, z)); model->diffuse.push_back(color);
    model->positions.push_back(glm::fvec3(ur.x, ll.y, z)); model->diffuse.push_back(color);
    // Others...
    for (int nIdx = 0; nIdx < 6; ++nIdx) model->normals.push_back(glm::fvec3(0.0f, 0.0f, 1.0f));
    for (int nIdx = 0; nIdx < 6; ++nIdx) model->ambient.push_back(glm::fvec3(0.0f, 0.0f, 0.0f));
}

// -------------------------------------------------------------------------------------------------
void pushRectOutline2d(Assets::Model *model, double t,
        const glm::fvec2 &ll, const glm::fvec2 &ur, const glm::fvec3 &color, float z = 0.0f)
{
    glm::fvec2 lr(ur.x, ll.y);
    glm::fvec2 ul(ll.x, ur.y);
    // Four edges
    pushRect2d(model, ll + glm::fvec2(-t, -t), ll + glm::fvec2( 0,  0), color, z); // LL
    pushRect2d(model, lr + glm::fvec2( 0, -t), lr + glm::fvec2(+t,  0), color, z); // LR
    pushRect2d(model, ul + glm::fvec2(-t,  0), ul + glm::fvec2( 0, +t), color, z); // UL
    pushRect2d(model, ur + glm::fvec2( 0,  0), ur + glm::fvec2(+t, +t), color, z); // UR
    // Four sides
    pushRect2d(model, ll + glm::fvec2( 0, -t), lr + glm::fvec2(+t,  0), color, z); // Bottom
    pushRect2d(model, ul + glm::fvec2( 0,  0), ur + glm::fvec2(+t, +t), color, z); // Top
    pushRect2d(model, ll + glm::fvec2(-t,  0), ul + glm::fvec2( 0,  0), color, z); // Left
    pushRect2d(model, lr + glm::fvec2( 0,  0), ur + glm::fvec2(+t,  0), color, z); // Right
}

// -------------------------------------------------------------------------------------------------
void decomposeTransform(const glm::fmat4 &xform,
    glm::fvec3 &translation, glm::fquat &rotation, glm::vec3 &scale)
{
    // Translation
    {
        translation = glm::fvec3(xform[3]);
    }
    // Rotation
    glm::fmat3 xform3 = glm::fmat3(xform);
    {
        // Normalize X, Y base vectors
        xform3[0] = glm::normalize(xform3[0]);
        xform3[1] = glm::normalize(xform3[1]);
        // Z from cross product to get ortho-normal base
        xform3[2] = glm::cross(xform3[0], xform3[1]);
        rotation = glm::quat_cast(xform3);
    }
    // Scale
    {
        scale.x = glm::length(xform3[0]);
        scale.y = glm::length(xform3[1]);
        scale.z = glm::length(xform3[2])
            * glm::dot(xform3[2], glm::fvec3(xform[2]));
    }
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(StateDb &sdb, Assets &assets)
{
    auto camera = sdb.create< Renderer::Camera::Info >(m_cameraHandle);
    camera->position = glm::fvec3(-10.0f, -20.0f, 20.0f);
    camera->target   = glm::fvec3(  0.0f,   0.0f,  5.0f);

    m_oceanModelAsset = assets.asset(
        "Procedural/Models/Ocean", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);
    m_uiModelAsset = assets.asset(
        "Procedural/Models/Ui", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);
    m_postModelAsset = assets.asset(
        "Procedural/Models/Post", Assets::Flag::PROCEDURAL);

    {
        auto mesh = sdb.create< Renderer::Mesh::Info >(m_arrowMeshHandle);
        mesh->modelAsset = assets.asset("Assets/Models/AxesY.model");
        mesh->groups = Renderer::Group::DEFAULT;
    }
    {
        auto mesh = sdb.create< Renderer::Mesh::Info >();
        mesh->modelAsset = m_postModelAsset;
        mesh->groups = Renderer::Group::DEFAULT_POST;
        auto model = assets.refModel(m_postModelAsset);
        pushRect2d(model,
            glm::fvec2(  0.0f,   0.0f),
            glm::fvec2(800.0f, 450.0f));
        // FIXME(martinmo): Encode texture coords into color attribute (hack!)
        model->diffuse[0] = model->diffuse[4] = glm::fvec3(0.0f, 0.0f, 0.0f);
        model->diffuse[1] = model->diffuse[3] = glm::fvec3(1.0f, 1.0f, 0.0f);
        model->diffuse[2] =                     glm::fvec3(0.0f, 1.0f, 0.0f);
        model->diffuse[5] =                     glm::fvec3(1.0f, 0.0f, 0.0f);
        model->setDefaultAttrs();
    }
    {
        auto mesh = sdb.create< Renderer::Mesh::Info >();
        mesh->modelAsset = m_uiModelAsset;
        mesh->groups = Renderer::Group::DEFAULT_GUI;
    }

    // Create simplistic tiled ocean
    {
        glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
        int   vertexCount = OCEAN_TILE_VERTEX_COUNT;
        float vertexDist  = OCEAN_TILE_VERTEX_DIST;
        Assets::Model *model = assets.refModel(m_oceanModelAsset);
        // Create ocean tile model
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
                float rand = glm::linearRand(+0.0f, +0.3f);
                glm::fvec3 color(0.4f + rand, 0.4f + rand, 0.4f + rand);
                for (int vIdx = 0; vIdx < 6; ++vIdx) model->diffuse.push_back(color);
                for (int vIdx = 0; vIdx < 6; ++vIdx) model->ambient.push_back(glm::fvec3(0.0));
            }
        }
        model->setDefaultAttrs();
        // Instantiate ocean tiles
        const int tileCount = 4;
        for (int y = 0; y < tileCount; ++y)
        {
            for (int x = 0; x < tileCount; ++x)
            {
                auto mesh = sdb.create< Renderer::Mesh::Info >();
                mesh->modelAsset = m_oceanModelAsset;
                mesh->translation = glm::fvec3(glm::fvec2(float(x), float(y))
                    * unitSize - 0.5f * float(tileCount) * unitSize, 0.0f);
                mesh->groups = Renderer::Group::DEFAULT;
            }
        }
    }

    // Create floating platform
    {
        // Create platform mesh
        u64 platformMeshHandle = 0;
        auto platformMesh = sdb.create< Renderer::Mesh::Info >(platformMeshHandle);
        platformMesh->translation = glm::fvec3(0.0f, 0.0f, 5.0f);
        platformMesh->rotation = glm::dquat(1.0f, 0.0f, 0.0f, 0.0f);
        platformMesh->modelAsset = assets.asset("Assets/Models/Platform.model");
        platformMesh->groups = Renderer::Group::DEFAULT;
        // Create platform rigid body
        u64 platformRigidBodyHandle = 0;
        auto platformRigidBody = sdb.create< Physics::RigidBody::Info >(platformRigidBodyHandle);
        platformRigidBody->mass = 10.0f;
        platformRigidBody->meshHandle = platformMeshHandle;
        platformRigidBody->collisionShape = Physics::RigidBody::CollisionShape::CONVEX_HULL_COMPOUND;
        platformRigidBody->collisionGroup = 1;
        platformRigidBody->collisionMask  = 1;
        // Create buoyancy spheres
        const float platformSize = 9.5f;
        for (int col = 0; col < PLATFORM_SPHERE_COUNT; ++col)
        {
            for (int row = 0; row < PLATFORM_SPHERE_COUNT; ++row)
            {
                glm::fvec3 translation = glm::fvec3(
                    -0.5f * platformSize + double(col)
                            / double(PLATFORM_SPHERE_COUNT - 1) * platformSize,
                    -0.5f * platformSize + double(row)
                            / double(PLATFORM_SPHERE_COUNT - 1) * platformSize, 0.0);
                addBuoyancyAffector(sdb, translation, platformRigidBodyHandle);
            }
        }
    }

    const bool enableRocket = true;
    for (int meshIdx = 0; meshIdx < 150; ++meshIdx)
    {
        u64 meshHandle = 0;
        auto mesh = sdb.create< Renderer::Mesh::Info >(meshHandle);
        mesh->translation = glm::linearRand(
            glm::fvec3(-15.0f, -15.0f, +20.0f), glm::fvec3(+15.0f, +15.0f, +40.0f));
        mesh->groups = Renderer::Group::DEFAULT;

        if (enableRocket && meshIdx == 0)
        {
            mesh->translation = glm::fvec3(0.0f, 0.0f, 10.0f);
            mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));

            mesh->modelAsset = assets.asset("Assets/Models/Pusher.model");
            //mesh->modelAsset = assets.asset("Assets/Models/PusherFront.model");
            //mesh->modelAsset = assets.asset("Assets/Models/InstanceTest.model");

            m_rocketModelAsset = mesh->modelAsset;

            // Create thrusters according to instances in model
            const Assets::Model *model = assets.refModel(mesh->modelAsset);
            for (const Assets::Model::Instance &instance : model->instances)
            {
                if (!Str::startsWith(instance.type, "Engine"))
                {
                    continue;
                }

                auto thruster = sdb.create< Thruster::Info >();
                thruster->shipMeshHandle = meshHandle;
                thruster->shipMeshInstanceNr = u32(&instance - &model->instances[0]) + 1;

                glm::fmat4 xform = instance.xform;

                decomposeTransform(xform, thruster->translation, thruster->rotation, thruster->scale);

                auto debugMesh = sdb.create< Renderer::Mesh::Info >(thruster->debugMeshHandle);
                debugMesh->modelAsset = assets.asset("Assets/Models/AxesXYZ.model");
                debugMesh->groups = Renderer::Group::DEFAULT;

                if (thruster->scale.x < 0.0
                    || thruster->scale.y < 0.0
                    || thruster->scale.z < 0.0)
                {
                    debugMesh->flags = Renderer::Mesh::Flag::SCALED;
                }
            }

            /*
            // For debugging the bloom effect...
            mesh->flags |= Renderer::Mesh::Flag::AMBIENT_ADD;
            mesh->ambientAdd = glm::fvec4(1.0, 0.0, 0.0, 0.0);
            */
        }
        else if (rand() % 9 > 5)
        {
            mesh->modelAsset = assets.asset("Assets/Models/MaterialCube.model");
        }
        else if (rand() % 9 > 2)
        {
            mesh->modelAsset = assets.asset("Assets/Models/Sphere.model");
        }
        else
        {
            mesh->modelAsset = assets.asset("Assets/Models/Torus.model");
        }

        m_sleepingMeshHandles.push_back(meshHandle);
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(StateDb &sdb)
{
    sdb.destroy(m_arrowMeshHandle);
    sdb.destroy(m_cameraHandle);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    PROFILER_SECTION(RocketScience, glm::fvec3(0.0f, 1.0f, 0.0f))

    m_timeInS += deltaTimeInS;

    // Update thruster debug meshes
    {
        auto thrusters = sdb.stateAll< Thruster::Info >();
        for (auto thruster : thrusters)
        {
            auto shipMesh  = sdb.state< Renderer::Mesh::Info >(thruster->shipMeshHandle);
            auto debugMesh = sdb.state< Renderer::Mesh::Info >(thruster->debugMeshHandle);
            debugMesh->translation = shipMesh->translation
                + shipMesh->rotation * thruster->translation;
            debugMesh->rotation = shipMesh->rotation * thruster->rotation;
            debugMesh->scale = thruster->scale;
        }
    }

    // Update tileable dynamic ocean model
    {
        PROFILER_SECTION(UpdateOcean, glm::fvec3(0.0f, 1.0f, 1.0f))

        Assets::Model *model = assets.refModel(m_oceanModelAsset);
        int vertexCount = int(model->positions.size());
        for (int vertexIdx = 0; vertexIdx < vertexCount; ++vertexIdx)
        {
            glm::fvec3 &position = model->positions[vertexIdx];
            glm::fvec2 pos2 = position.xy();
            position.z = oceanEquation(pos2, m_timeInS);
            glm::fvec3 oceanPtLeft (pos2 + glm::fvec2(-1.0f,  0.0f), 0.0f);
            oceanPtLeft.z  = oceanEquation(oceanPtLeft.xy(),  m_timeInS);
            glm::fvec3 oceanPtRight(pos2 + glm::fvec2(+1.0f,  0.0f), 0.0f);
            oceanPtRight.z = oceanEquation(oceanPtRight.xy(), m_timeInS);
            glm::fvec3 oceanPtAbove(pos2 + glm::fvec2( 0.0f, +1.0f), 0.0f);
            oceanPtAbove.z = oceanEquation(oceanPtAbove.xy(), m_timeInS);
            glm::fvec3 oceanPtBelow(pos2 + glm::fvec2( 0.0f, -1.0f), 0.0f);
            oceanPtBelow.z = oceanEquation(oceanPtBelow.xy(), m_timeInS);
            model->normals[vertexIdx] = glm::normalize(
                glm::cross(oceanPtRight - oceanPtLeft, oceanPtAbove - oceanPtBelow));
        }
    }

    // FIXME(martinmo): Add keyboard input to platform abstraction (remove dependency to SDL)
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    renderer.debugNormals = state[SDL_SCANCODE_N] != 0;
    bool rocketBoost = state[SDL_SCANCODE_SPACE] != 0;

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

        auto camera = sdb.state< Renderer::Camera::Info >(m_cameraHandle);

        glm::fvec3 cameraDir = (camera->target - camera->position).xyz();

        // Vertical rotation axis in world space
        // FIXME(martinmo): Correctly handle near +/- 90 deg cases
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
        camera->position = glm::fvec3(xform * glm::fvec4(camera->position, 1.0f));
        if (glm::abs(translationInM) > 0.001)
        {
            camera->position += glm::normalize(cameraDir) * float(translationInM);
        }
        renderer.activeCameraHandle = m_cameraHandle;
    }

    // Turn meshes into rigid bodies
    if (state[SDL_SCANCODE_P] && !m_sleepingMeshHandles.empty())
    {
        u64 meshHandle = m_sleepingMeshHandles.front();
        m_sleepingMeshHandles.pop_front();

        u64 rigidBodyHandle = 0;
        auto rigidBody = sdb.create< Physics::RigidBody::Info >(rigidBodyHandle);
        rigidBody->mass = 1.0f;
        rigidBody->meshHandle = meshHandle;
        rigidBody->collisionGroup = 1;
        rigidBody->collisionMask  = 1;

        auto mesh = sdb.state< Renderer::Mesh::Info >(meshHandle);

        //if (mesh->modelAsset == m_rocketModelAsset)
        if (mesh->modelAsset == assets.asset("Assets/Models/Pusher.model"))
        {
            rigidBody->collisionShape = Physics::RigidBody::CollisionShape::CONVEX_HULL_COMPOUND;
            rigidBody->mass = 5.0f;
            // Create Pusher rocket motor force
            {
                auto affector = sdb.create< Physics::Affector::Info >(m_pusherAffectorHandle);
                affector->rigidBodyHandle = rigidBodyHandle;
                affector->enabled = 1;
                //affector->forcePosition = glm::fvec3(0.00f, -2.64, 2.75f);   // Top wing
                //affector->forcePosition = glm::fvec3(0.00f,  3.61, 0.00f);   // Head
                affector->forcePosition = glm::fvec3(0.00f, -2.14, 0.00f);   // Main engine
            }
        }
        else if (mesh->modelAsset == assets.asset("Assets/Models/Sphere.model"))
        {
            rigidBody->collisionShape = Physics::RigidBody::CollisionShape::BOUNDING_SPHERE;
            addBuoyancyAffector(sdb, glm::fvec3(0.0, 0.0, 0.0), rigidBodyHandle);
        }
        else
        {
            addBuoyancyAffector(sdb, glm::fvec3(-0.5, -0.5, +0.0), rigidBodyHandle);
            addBuoyancyAffector(sdb, glm::fvec3(-0.5, +0.5, +0.0), rigidBodyHandle);
            addBuoyancyAffector(sdb, glm::fvec3(+0.5, +0.5, +0.0), rigidBodyHandle);
            addBuoyancyAffector(sdb, glm::fvec3(+0.5, -0.5, +0.0), rigidBodyHandle);
        }
    }

    // Update rocket force control logic
    if (m_pusherAffectorHandle)
    {
        auto affector = sdb.state< Physics::Affector::Info >(m_pusherAffectorHandle);
        auto rigidBody = sdb.state< Physics::RigidBody::Info >(affector->rigidBodyHandle);
        auto mesh = sdb.state< Renderer::Mesh::Info >(rigidBody->meshHandle);

        glm::fmat3 meshRot = glm::mat3_cast(mesh->rotation);

        /*
        // This force applied to top wing makes rocket spin clockwise
        affector->force = modelRot * glm::fvec3(10.0f, 0.0f, 0.0f);
        */

        glm::fvec3 nozzlePosition = mesh->translation + mesh->rotation * affector->forcePosition;

        // Main engine force to keep height of 10 m
        float mainEngineForce = 8.0f * rigidBody->mass;
        if (rocketBoost) mainEngineForce *= 2.0f;
        if (nozzlePosition.z < 5.0f && !rocketBoost) mainEngineForce = 0.0f;

        // Rocket's local +Y should be aligned to global +Z
        glm::fvec3 nominalDir = glm::fvec3(0.0, 0.0, 1.0);
        glm::fvec3 actualDir  = meshRot * glm::fvec3(0.0f, 1.0f, 0.0f);

        // Tilt nominal direction to steer rocket towards origin
        nominalDir += glm::normalize(-mesh->translation) / 20.0f;
        nominalDir  = glm::normalize(nominalDir);

        // Thrust vector tries to align rocket with nominal direction
        glm::fvec3 thrustVector = glm::normalize(nominalDir + 1.2f * (actualDir - nominalDir));

        // Add some main engine jitter
        glm::fvec3 maxNoise = glm::fvec3(0.05f, 0.05f, 0.05f);
        thrustVector += glm::linearRand(-maxNoise, maxNoise);

        affector->force = thrustVector * mainEngineForce;

        // Use arrow mesh to display thrust vector
        auto arrowMesh = sdb.state< Renderer::Mesh::Info >(m_arrowMeshHandle);
        arrowMesh->translation = nozzlePosition;
        arrowMesh->rotation = Math::rotateFromTo(
            glm::fvec3(0.0f, 1.0f, 0.0f), -affector->force);

        // Emergency motors off...
        double errorAngleInDeg = glm::degrees(
            glm::angle(Math::rotateFromTo(nominalDir, actualDir)));
        //Logger::debug("errorAngleInDeg: %5.2f", errorAngleInDeg);
        if (glm::abs(errorAngleInDeg) > 20.0f)
        {
            sdb.destroy(m_pusherAffectorHandle);
            m_pusherAffectorHandle = 0;
        }

        // Add rocket smoke particles
        m_rocketSmokeParticlesDelay -= deltaTimeInS;
        if (mainEngineForce < 0.1f) m_rocketSmokeParticlesDelay = 0.0;
        while (m_rocketSmokeParticlesDelay < 0.0)
        {
            u64 particleMeshHandle = 0;
            auto particleMesh = sdb.create< Renderer::Mesh::Info >(particleMeshHandle);
            particleMesh->translation = nozzlePosition;
            particleMesh->translation += -0.5f * glm::normalize(affector->force);
            particleMesh->modelAsset = assets.asset("Assets/Models/Sphere.model");
            particleMesh->groups = Renderer::Group::DEFAULT;
            particleMesh->flags |= Renderer::Mesh::Flag::SCALED;
            particleMesh->flags |= Renderer::Mesh::Flag::DIFFUSE_MUL;
            particleMesh->diffuseMul = glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f);
            particleMesh->flags |= Renderer::Mesh::Flag::AMBIENT_ADD;

            u64 particleHandle = 0;
            auto particle = sdb.create< Particle::Info >(particleHandle);
            particle->meshHandle = particleMeshHandle;
            particle->velocity = -affector->force / 3.0f;
            particle->minSize = 1.0f + glm::linearRand(-0.2f, +0.0f);
            if (rand() % 100 > 90) particle->maxSize = 5.0f + glm::linearRand(-1.0f, +1.5f);
            else                   particle->maxSize = 5.0f + glm::linearRand(-0.8f, +0.25f);

            double distBetweenParticlesInM = 0.5;
            double particleSpeed = double(glm::length(particle->velocity));
            m_rocketSmokeParticlesDelay += distBetweenParticlesInM / particleSpeed;
        }
    }

    // Update buoyancy affectors
    {
        updateBuoyancyAffectors(sdb, m_timeInS);
    }

    // Update rocket smoke particles
    {
        const float maxAgeInS = 2.0f;
        std::vector< u64 > particleHandlesToBeDeleted;
        auto particles = sdb.stateAll< Particle::Info >();
        for (auto particle : particles)
        {
            if (particle->ageInS >= maxAgeInS)
            {
                particleHandlesToBeDeleted.push_back(sdb.handleFromState(particle));
                continue;
            }

            auto mesh = sdb.state< Renderer::Mesh::Info >(particle->meshHandle);
            if (particle->ageInS > 0.0)
            {
                mesh->translation += particle->velocity * float(deltaTimeInS);
            }
            particle->velocity += -particle->velocity * 1.0f * float(deltaTimeInS);
            particle->ageInS += deltaTimeInS;

            const float life = glm::clamp(float(particle->ageInS) / maxAgeInS, 0.0f, 1.0f);
            const float grow   = 0.30f;
            const float shrink = 0.25f;
            float uniformScale = 1.0f;
            if (life >= 1.0f - shrink)
            {
                uniformScale = (1.0f - glm::smoothstep(1.0f - shrink, 1.0f, life))
                        * (particle->maxSize - particle->minSize);
                mesh->diffuseMul = glm::mix(
                    glm::fvec4(0.8, 0.8, 0.8, 1.0), glm::fvec4(0.1, 0.1, 0.1, 1.0),
                    (life - (1.0f - shrink)) / shrink);
            }
            else if (life <= grow)
            {
                uniformScale = particle->minSize
                        + glm::smoothstep(0.0f, grow, life)
                        * (particle->maxSize - particle->minSize);
                mesh->diffuseMul = glm::mix(
                    glm::fvec4(1.0, 0.5, 0.0, 1.0), glm::fvec4(0.8, 0.8, 0.8, 1.0),
                    life / grow);
                mesh->ambientAdd = glm::mix(
                    glm::fvec4(1.0, 0.5, 0.0, 0.0), glm::fvec4(0.0, 0.0, 0.0, 0.0),
                    life / grow);
            }
            else
            {
                uniformScale = particle->maxSize
                        - glm::smoothstep(grow, 1.0f - shrink, life)
                        * particle->minSize;
            }
            mesh->scale = glm::fvec3(uniformScale);

            float oceanHeight = oceanEquation(mesh->translation.xy(), m_timeInS);
            float minHeight = 0.3f * uniformScale + oceanHeight;
            if (mesh->translation.z < minHeight)
            {
                particle->velocity.z = 0.0f;
                mesh->translation.z = minHeight;
            }
        }
        for (auto &particleHandle : particleHandlesToBeDeleted)
        {
            auto particle = sdb.state< Particle::Info >(particleHandle);
            sdb.destroy(particle->meshHandle);
            sdb.destroy(particleHandle);
        }
    }

    // Update profiling UI
    {
        PROFILER_SECTION(UpdateUi, glm::fvec3(1.0f, 0.0f, 1.0f))

        auto uiModel = assets.refModel(m_uiModelAsset);

        uiModel->clear();

        Profiler *profiling = Profiler::instance();
        Profiler::Thread *mainThread = profiling->mainThreadPrevFrame();

        float offsPx   = 15.0f;
        float msPx     = 30.0f;
        float barPx    = 30.0f;
        float shrinkPx =  5.0f;
        float outlPx   =  1.0f;

        const float maxFt = 1000.0f / 60.0f;

        glm::fvec3 white(1.0f, 1.0f, 1.0f);
        glm::fvec3 black(0.0f, 0.0f, 0.0f);
        glm::fvec3 dgray(0.3f, 0.3f, 0.3f);
        glm::fvec3  gray(0.5f, 0.5f, 0.5f);
        glm::fvec3 red  (1.0f, 0.0f, 0.0f);

        if (mainThread)
        {
            for (Profiler::SectionSample &sample : mainThread->samples)
            {
                float enterMs   = float(profiling->ticksToMs(sample.ticksEnter));
                float exitMs    = float(profiling->ticksToMs(sample.ticksExit));
                float callDepth = float(sample.callDepth);

                glm::fvec2 ll(offsPx + msPx * enterMs, offsPx         + shrinkPx * callDepth);
                glm::fvec2 ur(offsPx + msPx * exitMs , offsPx + barPx - shrinkPx * callDepth);

                glm::fvec2 uro(ur + glm::fvec2(+outlPx, 0.0f));
                glm::fvec3 color = sample.section->color;

                pushRect2d       (uiModel,         ll, uro, color, callDepth);
                pushRectOutline2d(uiModel, outlPx, ll, ur , black, callDepth + 0.5f);
            }
        }
        for (int msIdx = 0; msIdx < 16; ++msIdx)
        {
            pushRectOutline2d(uiModel, outlPx,
                glm::fvec2(offsPx + msPx * (msIdx + 0) + outlPx, offsPx        ),
                glm::fvec2(offsPx + msPx * (msIdx + 1)         , offsPx + barPx), black, 5.0f);
        }
        pushRectOutline2d(uiModel, outlPx * 2.0f,
            glm::fvec2(offsPx                        , offsPx - outlPx        ),
            glm::fvec2(offsPx + msPx * maxFt + outlPx, offsPx + outlPx + barPx), red, 10.0f);

        uiModel->setDefaultAttrs();
    }
}

// -------------------------------------------------------------------------------------------------
#define __sin(arg) std::sin(arg)
//#define __sin(arg) glm::fastSin(arg)
//#define __sin(arg) (arg) // NOP for testing performance impact...
float RocketScience::oceanEquation(const glm::fvec2 &position, double timeInS)
{
    glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
    float twoPi = 2.0f * glm::pi< float >();
    float result = 0.0f;
    result += 0.50f * __sin(twoPi / (0.500f * unitSize.x)
        * float(position.x + position.y + timeInS));
    result += 0.50f * __sin(twoPi / (1.000f * unitSize.x)
        * float(position.x - position.y + timeInS));
    result += 0.10f * __sin(twoPi / (0.250f * unitSize.x)
        * float(position.x + position.y - timeInS * 0.5f));
    return result;
}
#undef __sin

// -------------------------------------------------------------------------------------------------
void RocketScience::addBuoyancyAffector(StateDb &sdb,
    const glm::fvec3 &translation, u64 parentRigidBodyHandle)
{
    u64 buoyancyAffectorHandle = 0;
    auto buoyancyAffector = sdb.create< Physics::Affector::Info >(buoyancyAffectorHandle);
    buoyancyAffector->rigidBodyHandle = parentRigidBodyHandle;
    buoyancyAffector->forcePosition = translation;
    m_buoyancyAffectorHandles.push_back(buoyancyAffectorHandle);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::updateBuoyancyAffectors(StateDb &sdb, double timeInS)
{
    // TODO(martinmo): Get rid of multi-level indirection by
    // TODO(martinmo): - Storing buoyancy type hint in force info
    // TODO(martinmo): - Keeping a copy of all relevant state in force info
    // TODO(martinmo):
    // TODO(martinmo): ==> Less efficient in space but more efficent in time
    // TODO(martinmo): ==> Flatten only if time-efficiency really is an issue

    auto rigidBodies = sdb.stateAll< Physics::RigidBody::Info >();
    std::vector< int > affectorCountByRigidBody(rigidBodies.endElem - rigidBodies.beginElem);

    // FIXME(martinmo): Buoyancy implementation clearly needs more work
    // FIXME(martinmo): ==> Cubes should have eight smaller affectors near corners
    // FIXME(martinmo): ==> Tori should have three/four small affectors inside tube

    for (auto buoyancyAffectorHandle : m_buoyancyAffectorHandles)
    {
        auto affector  = sdb.state< Physics::Affector::Info  >(buoyancyAffectorHandle);
        auto rigidBody = sdb.state< Physics::RigidBody::Info >(affector->rigidBodyHandle);

        int rigidBodyIdx = int(rigidBody - rigidBodies.beginElem);
        ++affectorCountByRigidBody[rigidBodyIdx];

        // Reset affector
        affector->enabled = 0;
        affector->force   = glm::fvec3(0.0f, 0.0f, 0.0f);
        affector->torque  = glm::fvec3(0.0f, 0.0f, 0.0f);

        // Reset RB linear velocity limit
        rigidBody->linearVelocityLimit.z = 0.0;
    }

    for (auto buoyancyAffectorHandle : m_buoyancyAffectorHandles)
    {
        auto affector  = sdb.state< Physics::Affector::Info  >(buoyancyAffectorHandle);
        auto rigidBody = sdb.state< Physics::RigidBody::Info >(affector->rigidBodyHandle);
        auto mesh      = sdb.state< Renderer::Mesh::Info     >(rigidBody->meshHandle);

        int rigidBodyIdx = int(rigidBody - rigidBodies.beginElem);
        int rigidBodyAffectorCount = affectorCountByRigidBody[rigidBodyIdx];

        glm::fvec3 affectorPosGlobal =
            mesh->translation + mesh->rotation * affector->forcePosition;

        glm::fvec2 pos2 = affectorPosGlobal.xy();
        glm::fvec3 oceanPt(pos2 + glm::fvec2(0.0f, 0.0f), 0.0f);
        oceanPt.z = oceanEquation(oceanPt.xy(), m_timeInS);

        double sphereRadius = 0.5;
        double sphereDepth = double(oceanPt.z - affectorPosGlobal.z) + sphereRadius;
        double spherePenetrationDepth = glm::clamp(sphereDepth, 0.0, 2.0 * sphereRadius);
        double spherePenetrationVolume = 0.0;
        if (spherePenetrationDepth > 0.0)
        {
            spherePenetrationVolume = (1.0 / 3.0) * glm::pi< double >()
                * glm::pow(spherePenetrationDepth, 2.0)
                * (3.0 * sphereRadius - spherePenetrationDepth);
        }

        if (spherePenetrationVolume <= 0.0)
        {
            continue;
        }

        glm::fvec3 oceanPtLeft (pos2 + glm::fvec2(-1.0f,  0.0f), 0.0f);
        oceanPtLeft.z  = oceanEquation(oceanPtLeft.xy(),  m_timeInS);
        glm::fvec3 oceanPtRight(pos2 + glm::fvec2(+1.0f,  0.0f), 0.0f);
        oceanPtRight.z = oceanEquation(oceanPtRight.xy(), m_timeInS);
        glm::fvec3 oceanPtAbove(pos2 + glm::fvec2( 0.0f, +1.0f), 0.0f);
        oceanPtAbove.z = oceanEquation(oceanPtAbove.xy(), m_timeInS);
        glm::fvec3 oceanPtBelow(pos2 + glm::fvec2( 0.0f, -1.0f), 0.0f);
        oceanPtBelow.z = oceanEquation(oceanPtBelow.xy(), m_timeInS);

        glm::fvec3 normal = glm::normalize(
            glm::cross(oceanPtRight - oceanPtLeft, oceanPtAbove - oceanPtBelow));

        // Force from buoyancy
        {
            // FIXME: Hard-coded factor should be derived from density of water
            affector->force += normal * float(spherePenetrationVolume * 50.0);
            affector->enabled = 1;
        }
        // Force from friction between moving RB and water
        {
            glm::fvec3 velocityAtAffector = rigidBody->linearVelocity
                + glm::cross(rigidBody->angularVelocity,
                    mesh->rotation * affector->forcePosition);
            glm::fvec3 frictionForce;
            frictionForce += -8.0f * glm::fvec3(velocityAtAffector.xy(), 0.0);
            frictionForce += -4.0f * glm::fvec3(0.0, 0.0, velocityAtAffector.z);

            // FIXME(martinmo): Only count affectors penetrating water surface
            //affector->force += frictionForce / float(rigidBodyAffectorCount);

            affector->force += frictionForce;
        }
        // Torque from friction between spinning RB and water
        {
            affector->torque += -0.05f * rigidBody->angularVelocity;
        }

        /*
        // Force from friction between spinning RB and water
        {
            glm::fvec3 avFriction = rigidBody->angularVelocity;
            avFriction.z = 0.0f;
            std::swap(avFriction.x, avFriction.y);
            avFriction.y = -avFriction.y;
            affector->force += 0.5f * avFriction;
        }
        */

        // FIXME(martinmo): Find better way of simulating effects of water-surface penetration
        // FIXME(martinmo): than the current hack using linear velocity limits (impulse-based?)
        rigidBody->linearVelocityLimit.z = 0.8f;
    }
}
