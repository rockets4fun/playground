// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#include "Physics.hpp"

#include <list>

#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Renderer.hpp"

struct Physics::PrivateRigidBody
{
    PrivateState &state;
    u64 objectHandle = 0;

    std::shared_ptr< btDefaultMotionState > motionState;
    std::shared_ptr< btRigidBody > rigidBody;

    PrivateRigidBody(PrivateState &state);
    virtual ~PrivateRigidBody();

    void initialize(u64 objectHandle, Physics::RigidBody::Info *info,
        Renderer::Mesh::Info *meshInfo);
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

    std::list< PrivateRigidBody > rigidBodies;
};

struct Physics::RigidBody::PrivateInfo
{
    static u64 STATE;
    PrivateRigidBody *rigidBody = nullptr;
};

u64 Physics::RigidBody::TYPE = 0;
u64 Physics::RigidBody::Info::STATE = 0;
u64 Physics::RigidBody::PrivateInfo::STATE = 0;

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
void Physics::PrivateRigidBody::initialize(u64 objectHandle,
    Physics::RigidBody::Info *info, Renderer::Mesh::Info *meshInfo)
{
    this->objectHandle = objectHandle;

    btScalar mass = 20;
    btVector3 inertia(0, 0, 0);
    state.cubeShape->calculateLocalInertia(mass, inertia);

    btQuaternion rotation(0.0, 0.0, 0.0, 1.0);
        /*
        meshInfo->orientation.x, meshInfo->orientation.y,
        meshInfo->orientation.z, meshInfo->orientation.w);
        */
    btVector3 translation(
        meshInfo->position.x, meshInfo->position.y, meshInfo->position.y);
    motionState = std::make_shared< btDefaultMotionState >(btTransform(rotation, translation));

    btRigidBody::btRigidBodyConstructionInfo rbConstructionInfo(
        mass, motionState.get(), state.cubeShape.get(), inertia);
    rbConstructionInfo.m_restitution = btScalar(0.5);
    rbConstructionInfo.m_friction    = btScalar(0.8);

    rigidBody = std::make_shared< btRigidBody >(rbConstructionInfo);

    state.dynamicsWorld->addRigidBody(rigidBody.get());
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
    RigidBody::TYPE = stateDb.registerType("RigidBody", 1024);
    RigidBody::Info::STATE = stateDb.registerState(
        RigidBody::TYPE, "Info", sizeof(RigidBody::Info));
    RigidBody::PrivateInfo::STATE = stateDb.registerState(
        RigidBody::TYPE, "PrivateInfo", sizeof(RigidBody::PrivateInfo));
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
        state->groundShape = std::make_shared< btStaticPlaneShape >(btVector3(0, 0, 1), 1.0f);
        state->groundMotionState = std::make_shared< btDefaultMotionState> (
            btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, -1)));
        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0,
            state->groundMotionState.get(), state->groundShape.get(), btVector3(0, 0, 0));
        groundRigidBodyCI.m_restitution = btScalar(0.5);
        groundRigidBodyCI.m_friction    = btScalar(0.8);
        state->groundRigidBody = std::make_shared< btRigidBody >(groundRigidBodyCI);
        state->dynamicsWorld->addRigidBody(state->groundRigidBody.get());
    }

    {
        state->cubeShape = std::make_shared< btBoxShape >(btVector3(0.5, 0.5, 0.5));
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Physics::shutdown(Platform &platform)
{
    state->dynamicsWorld->removeRigidBody(state->groundRigidBody.get());

    state = nullptr;
}

// -------------------------------------------------------------------------------------------------
void Physics::update(Platform &platform, double deltaTimeInS)
{
    // Check for newly created rigid bodies
    RigidBody::Info *info, *infoBegin, *infoEnd;
    platform.stateDb.fullState(RigidBody::Info::STATE, &infoBegin, &infoEnd);
    RigidBody::PrivateInfo *privateInfo, *privateInfoBegin;
    platform.stateDb.fullState(RigidBody::PrivateInfo::STATE, &privateInfoBegin);

    for (info = infoBegin, privateInfo = privateInfoBegin;
         info != infoEnd; ++info, ++privateInfo)
    {
        Renderer::Mesh::Info *meshInfo;
        platform.stateDb.state(Renderer::Mesh::Info::STATE, info->meshObjectHandle, &meshInfo);
        if (!privateInfo->rigidBody)
        {
            u64 objectHandle = platform.stateDb.objectHandleFromElem(
                Physics::RigidBody::Info::STATE, info);
            state->rigidBodies.push_back(PrivateRigidBody(*state.get()));
            PrivateRigidBody *rigidBody = &state->rigidBodies.back();
            rigidBody->initialize(objectHandle, info, meshInfo);
            privateInfo->rigidBody = rigidBody;
        }
    }

    int internalSteps = state->dynamicsWorld->stepSimulation(
        btScalar(deltaTimeInS), 10, btScalar(1.0 / 100.0));

    for (info = infoBegin, privateInfo = privateInfoBegin;
         info != infoEnd; ++info, ++privateInfo)
    {
        Renderer::Mesh::Info *meshInfo;
        platform.stateDb.state(Renderer::Mesh::Info::STATE, info->meshObjectHandle, &meshInfo);
        btTransform worldTrans;
        privateInfo->rigidBody->motionState->getWorldTransform(worldTrans);
        btVector3 origin = worldTrans.getOrigin();
        meshInfo->position =
            glm::fvec4(origin.x(), origin.y(), origin.z(), 1.0);
        btQuaternion rotation = worldTrans.getRotation();
        meshInfo->orientation =
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
