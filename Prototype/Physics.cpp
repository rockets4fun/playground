// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#include "Physics.hpp"

#include <list>

#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Platform.hpp"
#include "Renderer.hpp"

struct Physics::PrivateRigidBody
{
    // FIXME(martinmo): Member-variables of structs/classes with methods
    // FIXME(martinmo): other than c'tor/d'tor should start with 'm_' prefix

    PrivateState &state;

    u64 objectHandle = 0;

    std::shared_ptr< btDefaultMotionState > motionState;
    std::shared_ptr< btRigidBody > rigidBody;

    std::vector< std::pair< glm::fvec3, glm::fvec3 > > forces;

    PrivateRigidBody(PrivateState &state);
    virtual ~PrivateRigidBody();

    void initialize(u64 objectHandle, Physics::RigidBody::Info *info,
        Renderer::Mesh::Info *meshInfo, const Assets::Model *model);
};

struct Physics::PrivateState
{
    std::shared_ptr< btBroadphaseInterface > broadphase;
    std::shared_ptr< btDefaultCollisionConfiguration > collisionConfiguration;
    std::shared_ptr< btCollisionDispatcher > dispatcher;

    std::shared_ptr< btSequentialImpulseConstraintSolver > solver;
    std::shared_ptr< btDiscreteDynamicsWorld > dynamicsWorld;

    std::shared_ptr< btCollisionShape > groundShape;
    std::shared_ptr< btDefaultMotionState > groundMotionState;
    std::shared_ptr< btRigidBody > groundRigidBody;

    std::shared_ptr< btCollisionShape > cubeShape;

    std::map< u64, std::shared_ptr< btCollisionShape > > collisionShapes;

    std::list< PrivateRigidBody > rigidBodies;

    static void preTickCallback(btDynamicsWorld *world, btScalar timeStep);
    void preTick(btScalar timeStep);
};

struct Physics::RigidBody::PrivateInfo
{
    static u64 STATE;
    PrivateRigidBody *rigidBody = nullptr;
};

u64 Physics::RigidBody::TYPE = 0;
u64 Physics::RigidBody::Info::STATE = 0;
u64 Physics::RigidBody::PrivateInfo::STATE = 0;

u64 Physics::Force::TYPE = 0;
u64 Physics::Force::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::PrivateRigidBody(PrivateState &state) :
    state(state)
{
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::~PrivateRigidBody()
{
    if (rigidBody)
    {
        state.dynamicsWorld->removeRigidBody(rigidBody.get());
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateRigidBody::initialize(
    u64 objectHandle, Physics::RigidBody::Info *info,
    Renderer::Mesh::Info *meshInfo, const Assets::Model *model)
{
    this->objectHandle = objectHandle;

    btCollisionShape *collisionShape = nullptr;
    u64 collisionShapeKey = u64(info->collisionShapeType) << 32 | u64(meshInfo->modelAsset);
    // TODO(martinmo): Find way to get rid of map lookup
    auto collisionShapeIter = state.collisionShapes.find(collisionShapeKey);
    if (collisionShapeIter == state.collisionShapes.end())
    {
        COMMON_ASSERT(model != nullptr);
        glm::fvec3 min(
            std::numeric_limits< float >::max(),
            std::numeric_limits< float >::max(),
            std::numeric_limits< float >::max());
        glm::fvec3 max(
            std::numeric_limits< float >::lowest(),
            std::numeric_limits< float >::lowest(),
            std::numeric_limits< float >::lowest());
        for (const auto &position : model->positions)
        {
            min = glm::min(min, position);
            max = glm::max(max, position);
        }
        glm::fvec3 halfExtent = 0.5f * (max - min);
        if (info->collisionShapeType == Physics::RigidBody::Info::BOUNDING_BOX)
        {
            collisionShape = new btBoxShape(btVector3(halfExtent.x, halfExtent.y, halfExtent.z));
        }
        else if (info->collisionShapeType == Physics::RigidBody::Info::BOUNDING_SPHERE)
        {
            collisionShape = new btSphereShape(glm::max(
                glm::max(halfExtent.x, halfExtent.y), halfExtent.z));
        }
        if (collisionShape)
        {
            state.collisionShapes[collisionShapeKey] =
                std::shared_ptr< btCollisionShape >(collisionShape);
        }
        else
        {
            // TODO(martinmo): Collision shape fallback warning?
            collisionShape = state.cubeShape.get();
        }
    }
    else
    {
        collisionShape = collisionShapeIter->second.get();
    }

    // TODO(martinmo): Update collision shape if asset version changes

    // Make sure initial rotation is a sane value
    if (glm::abs(meshInfo->rotation.x) + glm::abs(meshInfo->rotation.x)
        + glm::abs(meshInfo->rotation.x) + glm::abs(meshInfo->rotation.x) < 0.01f)
    {
        meshInfo->rotation = glm::fquat(1.0, 0.0, 0.0, 0.0);
    }
    btQuaternion rotation(
        meshInfo->rotation.x, meshInfo->rotation.y,
        meshInfo->rotation.z, meshInfo->rotation.w);
    btVector3 translation(
        meshInfo->translation.x, meshInfo->translation.y, meshInfo->translation.z);
    motionState = std::make_shared< btDefaultMotionState >(btTransform(rotation, translation));

    btScalar mass = 5;
    btVector3 inertia(0, 0, 0);
    collisionShape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass, motionState.get(), collisionShape, inertia);
    constructionInfo.m_restitution = btScalar(0.25);
    constructionInfo.m_friction = btScalar(0.2);
    constructionInfo.m_rollingFriction = btScalar(0.2);
    rigidBody = std::make_shared< btRigidBody >(constructionInfo);

    // TODO(martinmo): Add flag to disable this for certain RBs only...
    rigidBody->setActivationState(DISABLE_DEACTIVATION);

    state.dynamicsWorld->addRigidBody(rigidBody.get());
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    PrivateState *state = (PrivateState *)world->getWorldUserInfo();
    state->preTick(timeStep);
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTick(btScalar timeStep)
{
    for (auto &privRigidBody : rigidBodies)
    {
        privRigidBody.rigidBody->clearForces();
        privRigidBody.rigidBody->applyGravity();
        for (const auto &force : privRigidBody.forces)
        {
            btVector3 bulletForce(force.first.x, force.first.y, force.first.z);
            btVector3 bulletForcePosition(force.second.x, force.second.y, force.second.z);
            privRigidBody.rigidBody->applyForce(bulletForce, bulletForcePosition);
        }
    }
}

// -------------------------------------------------------------------------------------------------
Physics::Physics()
{
}

// -------------------------------------------------------------------------------------------------
Physics::~Physics()
{
}

// -------------------------------------------------------------------------------------------------
void Physics::registerTypesAndStates(StateDb &stateDb)
{
    RigidBody::TYPE = stateDb.registerType("RigidBody", 1025);
    RigidBody::Info::STATE = stateDb.registerState(
        RigidBody::TYPE, "Info", sizeof(RigidBody::Info));
    RigidBody::PrivateInfo::STATE = stateDb.registerState(
        RigidBody::TYPE, "PrivateInfo", sizeof(RigidBody::PrivateInfo));

    Force::TYPE = stateDb.registerType("Force", 256);
    Force::Info::STATE = stateDb.registerState(
        Force::TYPE, "Info", sizeof(Force::Info));
}

// -------------------------------------------------------------------------------------------------
bool Physics::initialize(Platform &platform)
{
    state = std::make_shared< PrivateState >();

    state->collisionConfiguration = std::make_shared< btDefaultCollisionConfiguration >();
    state->dispatcher = std::make_shared< btCollisionDispatcher >(
        state->collisionConfiguration.get());

    state->broadphase = std::make_shared< btDbvtBroadphase >();
    state->solver = std::make_shared< btSequentialImpulseConstraintSolver >();

    state->dynamicsWorld = std::make_shared< btDiscreteDynamicsWorld >(
        state->dispatcher.get(), state->broadphase.get(),
        state->solver.get(), state->collisionConfiguration.get());

    state->dynamicsWorld->setGravity(btVector3(0, 0, -10));

    {
        state->groundShape = std::make_shared< btStaticPlaneShape >(btVector3(0, 0, 1), 0.0f);
        state->groundMotionState = std::make_shared< btDefaultMotionState> (
            btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0,
            state->groundMotionState.get(), state->groundShape.get());
        groundRigidBodyCI.m_restitution = btScalar(1.0);
        groundRigidBodyCI.m_friction = btScalar(1.0);
        groundRigidBodyCI.m_rollingFriction = btScalar(1.0);
        state->groundRigidBody = std::make_shared< btRigidBody >(groundRigidBodyCI);
        state->dynamicsWorld->addRigidBody(state->groundRigidBody.get());
    }

    {
        state->cubeShape = std::make_shared< btBoxShape >(btVector3(0.5, 0.5, 0.5));
    }

    //state->dynamicsWorld->setSynchronizeAllMotionStates(...)
    state->dynamicsWorld->setInternalTickCallback(
        PrivateState::preTickCallback, state.get(), true);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Physics::shutdown(Platform &platform)
{
    state->dynamicsWorld->setInternalTickCallback(nullptr);
    state->dynamicsWorld->removeRigidBody(state->groundRigidBody.get());
    state = nullptr;
}

// -------------------------------------------------------------------------------------------------
void Physics::update(Platform &platform, double deltaTimeInS)
{
    RigidBody::Info *info = nullptr, *infoBegin = nullptr, *infoEnd = nullptr;
    platform.stateDb.refStateAll(RigidBody::Info::STATE, &infoBegin, &infoEnd);
    RigidBody::PrivateInfo *privateInfo = nullptr, *privateInfoBegin = nullptr;
    platform.stateDb.refStateAll(RigidBody::PrivateInfo::STATE, &privateInfoBegin);

    // Check for newly created rigid bodies
    for (info = infoBegin, privateInfo = privateInfoBegin;
         info != infoEnd; ++info, ++privateInfo)
    {
        Renderer::Mesh::Info *meshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, info->meshHandle, &meshInfo);
        if (!privateInfo->rigidBody)
        {
            u64 objectHandle = platform.stateDb.objectHandleFromElem(
                Physics::RigidBody::Info::STATE, info);
            state->rigidBodies.push_back(PrivateRigidBody(*state.get()));
            PrivateRigidBody *rigidBody = &state->rigidBodies.back();

            const Assets::Model *model = platform.assets.refModel(meshInfo->modelAsset);
            rigidBody->initialize(objectHandle, info, meshInfo, model);

            privateInfo->rigidBody = rigidBody;
        }
        privateInfo->rigidBody->forces.clear();
    }

    Force::Info *forceInfo = nullptr, *forceInfoBegin = nullptr, *forceInfoEnd = nullptr;
    platform.stateDb.refStateAll(Force::Info::STATE, &forceInfoBegin, &forceInfoEnd);
    for (forceInfo = forceInfoBegin; forceInfo != forceInfoEnd; ++forceInfo)
    {
        platform.stateDb.refState(RigidBody::PrivateInfo::STATE,
            forceInfo->rigidBodyHandle, &privateInfo);
        if (forceInfo->enabled)
        {
            privateInfo->rigidBody->forces.push_back(
                std::make_pair(forceInfo->force, forceInfo->position));
        }
    }

    // Update rigid body physics simulation
    state->dynamicsWorld->stepSimulation(btScalar(deltaTimeInS), 5, btScalar(1.0 / 60));

    // TODO(martinmo): Use 'CProfileIterator' to present profiling data in meaningful way
    /*
#ifndef NDEBUG
    if (m_nextPrintProfilingEventInS > 0.0)
    {
        m_nextPrintProfilingEventInS -= deltaTimeInS;
        if (m_nextPrintProfilingEventInS <= 0.0)
        {
            CProfileManager::dumpAll();
            m_nextPrintProfilingEventInS = 1.0;
        }
    }
#endif
    */

    for (info = infoBegin, privateInfo = privateInfoBegin;
         info != infoEnd; ++info, ++privateInfo)
    {
        // TODO(martinmo): Delete rigid bodies with bad mesh references?
        // TODO(martinmo): (e.g. mesh has been destroyed and rigid body still alive)
        if (!platform.stateDb.isObjectHandleValid(info->meshHandle))
        {
            continue;
        }

        Renderer::Mesh::Info *meshInfo = nullptr;
        platform.stateDb.refState(Renderer::Mesh::Info::STATE, info->meshHandle, &meshInfo);

        btTransform worldTrans;
        privateInfo->rigidBody->motionState->getWorldTransform(worldTrans);

        btVector3 origin = worldTrans.getOrigin();
        meshInfo->translation =
            glm::fvec4(origin.x(), origin.y(), origin.z(), 1.0);

        btQuaternion rotation = worldTrans.getRotation();
        meshInfo->rotation =
            glm::fquat(rotation.w(), rotation.x(), rotation.y(), rotation.z());
    }

    // Check for destroyed rigid bodies
    std::list< PrivateRigidBody >::iterator rigidBodiesIter;
    std::vector< decltype(rigidBodiesIter) > deletions;
    for (rigidBodiesIter = state->rigidBodies.begin();
         rigidBodiesIter != state->rigidBodies.end();
         ++rigidBodiesIter)
    {
        if (!platform.stateDb.isObjectHandleValid(rigidBodiesIter->objectHandle))
        {
            deletions.push_back(rigidBodiesIter);
        }
    }
    while (!deletions.empty())
    {
        state->rigidBodies.erase(deletions.back());
        deletions.pop_back();
    }
}
