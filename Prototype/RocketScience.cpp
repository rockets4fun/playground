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

#include "Math.hpp"
#include "Logging.hpp"
#include "Profiling.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"

#ifdef COMMON_WINDOWS
#   include <Brofiler.h>
#endif

const int RocketScience::OCEAN_TILE_VERTEX_COUNT = 32;
const glm::fvec2 RocketScience::OCEAN_TILE_UNIT_SIZE =
    glm::fvec2((OCEAN_TILE_VERTEX_COUNT - 1) * OCEAN_TILE_VERTEX_DIST);
const float RocketScience::OCEAN_TILE_VERTEX_DIST = 1.0f;

const int RocketScience::PLATFORM_SPHERE_COUNT = 4;

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
void pushRect2d(Assets::Model *model,
        const glm::fvec2 &ll, const glm::fvec2 &ur, const glm::fvec3 &color, float z = 0.0f)
{
    // World coordinate system: +X=E, +Y=N and +Z=up (front facing is CCW towards negative axis)
    // Lower left triangle
    model->positions.push_back(glm::fvec3(ll.x, ll.y, z)); model->colors.push_back(color);
    model->positions.push_back(glm::fvec3(ur.x, ur.y, z)); model->colors.push_back(color);
    model->positions.push_back(glm::fvec3(ll.x, ur.y, z)); model->colors.push_back(color);
    // Upper right triangle
    model->positions.push_back(glm::fvec3(ur.x, ur.y, z)); model->colors.push_back(color);
    model->positions.push_back(glm::fvec3(ll.x, ll.y, z)); model->colors.push_back(color);
    model->positions.push_back(glm::fvec3(ur.x, ll.y, z)); model->colors.push_back(color);
    // Normals...
    for (int nIdx = 0; nIdx < 6; ++nIdx) model->normals.push_back(glm::fvec3(0.0f, 0.0f, 1.0f));
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(StateDb &sdb, Assets &assets)
{
    auto camera = sdb.create< Renderer::Camera::Info >(m_cameraHandle);
    camera->position = glm::fvec3(-10.0f, -20.0f, 20.0f);
    camera->target   = glm::fvec3(  0.0f,   0.0f,  0.0f);

    m_oceanModelAsset = assets.asset(
        "procedural/ocean", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);
    m_uiModelAsset = assets.asset(
        "procedural/ui", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);

    {
        auto mesh = sdb.create< Renderer::Mesh::Info >(m_arrowMeshHandle);
        mesh->modelAsset = assets.asset("Assets/Arrow.obj");
    }

    {
        auto mesh = sdb.create< Renderer::Mesh::Info >(m_uiMeshHandle);
        mesh->modelAsset = m_uiModelAsset;
        mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
    }

    // Create floating platform
    {
        // Create platform mesh
        u64 platformMeshHandle = 0;
        auto platformMesh = sdb.create< Renderer::Mesh::Info >(platformMeshHandle);
        platformMesh->translation = glm::fvec3(0.0f, 0.0f, 5.0f);
        platformMesh->rotation = glm::dquat(1.0f, 0.0f, 0.0f, 0.0f);
        platformMesh->modelAsset = assets.asset("Assets/Platform.obj");
        // Create platform rigid body
        u64 platformRigidBodyHandle = 0;
        auto platformRigidBody = sdb.create< Physics::RigidBody::Info >(platformRigidBodyHandle);
        platformRigidBody->mass = 10.0f;
        platformRigidBody->meshHandle = platformMeshHandle;
        platformRigidBody->collisionShapeType = Physics::CollisionShapeType::CONVEX_HULL_COMPOUND;
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
    for (int meshIdx = 0; meshIdx < 128; ++meshIdx)
    {
        u64 meshHandle = 0;
        auto mesh = sdb.create< Renderer::Mesh::Info >(meshHandle);
        mesh->translation = glm::linearRand(
            glm::fvec3(-10.0f, -10.0f, +20.0f), glm::fvec3(+10.0f, +10.0f, +40.0f));

        if (enableRocket && meshIdx == 0)
        {
            mesh->translation = glm::fvec3(0.0f, 0.0f, 10.0f);
            mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
            mesh->modelAsset = assets.asset("Assets/Pusher.obj");
        }
        else if (rand() % 9 > 5)
        {
            mesh->modelAsset = assets.asset("Assets/MaterialCube.obj");
        }
        else if (rand() % 9 > 2)
        {
            mesh->modelAsset = assets.asset("Assets/Sphere.obj");
        }
        else
        {
            mesh->modelAsset = assets.asset("Assets/Torus.obj");
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
//#ifdef COMMON_WINDOWS
//    BROFILER_CATEGORY("RocketScience", Profiler::Color::Green)
//#endif
    PROFILING_SECTION(RocketScience, glm::fvec3(0.0f, 1.0f, 0.0f))

    m_timeInS += deltaTimeInS;

    // Update tileable dynamic ocean model
    {
//#ifdef COMMON_WINDOWS
//        BROFILER_CATEGORY("UpdateOcean", Profiler::Color::Gray)
//#endif
        PROFILING_SECTION(UpdateOcean, glm::fvec3(0.0f, 1.0f, 1.0f))

        glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
        int vertexCount = OCEAN_TILE_VERTEX_COUNT;
        float vertexDist = OCEAN_TILE_VERTEX_DIST;
        Assets::Model *model = assets.refModel(m_oceanModelAsset);
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
                    float rand = glm::linearRand(0.0f, +0.3f);
                    glm::fvec3 color(0.4f + rand, 0.4f + rand, 0.4f + rand);
                    for (int vIdx = 0; vIdx < 6; ++vIdx) model->colors.push_back(color);
                }
            }
            // Create ocean tiles
            const int tileCount = 4;
            for (int y = 0; y < tileCount; ++y)
            {
                for (int x = 0; x < tileCount; ++x)
                {
                    auto mesh = sdb.create< Renderer::Mesh::Info >();
                    mesh->modelAsset = m_oceanModelAsset;
                    mesh->translation = glm::fvec3(glm::fvec2(float(x), float(y))
                        * unitSize - 0.5f * float(tileCount) * unitSize, 0.0f);
                }
            }
        }

        // Update positions and normals according to ocean equation
        vertexCount = int(model->positions.size());
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

        if (mesh->modelAsset == assets.asset("Assets/Pusher.obj"))
        {
            rigidBody->collisionShapeType = Physics::CollisionShapeType::CONVEX_HULL_COMPOUND;
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
        else if (mesh->modelAsset == assets.asset("Assets/Sphere.obj"))
        {
            rigidBody->collisionShapeType = Physics::CollisionShapeType::BOUNDING_SPHERE;
            addBuoyancyAffector(sdb, glm::fvec3(0.0, 0.0, 0.0), rigidBodyHandle);
        }
        else
        {
            addBuoyancyAffector(sdb, glm::fvec3(+0.5, -0.5, +0.0), rigidBodyHandle);
            addBuoyancyAffector(sdb, glm::fvec3(+0.5, +0.5, +0.0), rigidBodyHandle);
            addBuoyancyAffector(sdb, glm::fvec3(-0.5, +0.0, +0.0), rigidBodyHandle);
        }
    }

    // Update rocket force control logic
    if (m_pusherAffectorHandle)
    {
        auto affector = sdb.state<
                Physics::Affector::Info >(m_pusherAffectorHandle);
        auto rigidBody = sdb.state<
                Physics::RigidBody::Info >(affector->rigidBodyHandle);
        auto mesh = sdb.state<
                Renderer::Mesh::Info >(rigidBody->meshHandle);

        glm::fmat3 meshRot = glm::mat3_cast(mesh->rotation);

        /*
        // This force applied to top wing makes rocket spin clockwise
        affector->force = modelRot * glm::fvec3(10.0f, 0.0f, 0.0f);
        */

        // Main engine force to keep height of 10 m
        float mainEngineForce = 0.0f;
        if (mesh->translation.z < 10.0)
        {
            mainEngineForce = 10.0f * rigidBody->mass;
        }
        else
        {
            mainEngineForce = 9.0f * rigidBody->mass;
        }

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
        arrowMesh->translation = mesh->translation + mesh->rotation * affector->forcePosition;
        arrowMesh->rotation = Math::rotateFromTo(
            glm::fvec3(0.0f, 1.0f, 0.0f), -affector->force);

        // Emergency motors off...
        double errorAngleInDeg = glm::degrees(
            glm::angle(Math::rotateFromTo(nominalDir, actualDir)));
        //Logging::debug("errorAngleInDeg: %5.2f", errorAngleInDeg);
        if (glm::abs(errorAngleInDeg) > 20.0f)
        {
            sdb.destroy(m_pusherAffectorHandle);
            m_pusherAffectorHandle = 0;
        }
    }

    // Update buoyancy affectors
    {
//#ifdef COMMON_WINDOWS
//        BROFILER_CATEGORY("UpdateBuoyancy", Profiler::Color::Gray)
//#endif
//        PROFILING_SECTION(UpdateBuoyancy, glm::fvec3(0.5f, 0.5f, 0.5f))

        updateBuoyancyAffectors(sdb, m_timeInS);
    }

    // Update profiling UI
    {
        PROFILING_SECTION(UpdateUi, glm::fvec3(1.0f, 0.0f, 1.0f))

        auto uiMesh = sdb.state< Renderer::Mesh::Info >(m_uiMeshHandle);
        auto uiModel = assets.refModel(uiMesh->modelAsset);
        uiModel->positions.clear(); uiModel->normals.clear(); uiModel->colors.clear();

        Profiling *profiling = Profiling::instance();
        Profiling::Thread *mainThread = profiling->mainThreadPrevFrame();
        if (mainThread)
        {
            for (Profiling::SectionSample &sample : mainThread->samples)
            {
                float enterMs = float(2.0 * profiling->ticksToMs(sample.ticksEnter));
                float exitMs  = float(2.0 * profiling->ticksToMs(sample.ticksExit));
                float bottom  = sample.callDepth * 2.0f;
                pushRect2d(uiModel,
                   glm::fvec2(enterMs, bottom),
                   glm::fvec2(exitMs, bottom + 1.5f), sample.section->color, 0.0f);
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------
float RocketScience::oceanEquation(const glm::fvec2 &position, double timeInS)
{
    glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
    float twoPi = 2.0f * glm::pi< float >();
    float result = 0.0f;
    result += 0.50f * sinf((twoPi / (0.500f * unitSize.x))
        * float(position.x + position.y + timeInS));
    result += 0.50f * sinf((twoPi / (1.000f * unitSize.x))
        * float(position.x - position.y + timeInS));
    result += 0.10f * sinf((twoPi / (0.250f * unitSize.x))
        * float(position.x + position.y - timeInS * 0.5f));
    return result;
}

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
void RocketScience::updateBuoyancyAffectors(StateDb &stateDb, double timeInS)
{
    // TODO(martinmo): Get rid of multi-level indirection by
    // TODO(martinmo): - Storing buoyancy type hint in force info
    // TODO(martinmo): - Keeping a copy of all relevant state in force info
    // TODO(martinmo):
    // TODO(martinmo): ==> Less efficient in space but more efficent in time
    // TODO(martinmo): ==> Flatten only if time-efficiency really is an issue

    for (auto buoyancyAffectorHandle : m_buoyancyAffectorHandles)
    {
        auto affector  = stateDb.state< Physics::Affector::Info  >(buoyancyAffectorHandle);
        auto rigidBody = stateDb.state< Physics::RigidBody::Info >(affector->rigidBodyHandle);

        // Reset affector
        affector->enabled = 0;
        affector->force  = glm::fvec3(0.0f, 0.0f, 0.0f);
        affector->torque = glm::fvec3(0.0f, 0.0f, 0.0f);

        // Reset RB linear velocity limit
        rigidBody->linearVelocityLimit.z = 0.0;
    }

    for (auto buoyancyAffectorHandle : m_buoyancyAffectorHandles)
    {
        auto affector  = stateDb.state< Physics::Affector::Info  >(buoyancyAffectorHandle);
        auto rigidBody = stateDb.state< Physics::RigidBody::Info >(affector->rigidBodyHandle);
        auto mesh      = stateDb.state< Renderer::Mesh::Info     >(rigidBody->meshHandle);

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
            affector->force += -8.0f * glm::fvec3(velocityAtAffector.xy(), 0.0);
            affector->force += -4.0f * glm::fvec3(0.0, 0.0, velocityAtAffector.z);
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
