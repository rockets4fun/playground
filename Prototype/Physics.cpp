// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#include "Physics.hpp"

#include <btBulletDynamicsCommon.h>

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Renderer.hpp"

struct Physics::RigidBody::InternalInfo
{
    static size_t STATE;
    btRigidBody *rigidBody = nullptr;
};

size_t Physics::RigidBody::TYPE = 0;
size_t Physics::RigidBody::Info::STATE = 0;
size_t Physics::RigidBody::InternalInfo::STATE = 0;

struct Physics::State
{
    std::shared_ptr< btBroadphaseInterface >           broadphase;
    std::shared_ptr< btDefaultCollisionConfiguration > collisionConfiguration;
    std::shared_ptr< btCollisionDispatcher >           dispatcher;

    std::shared_ptr< btSequentialImpulseConstraintSolver > solver;
    std::shared_ptr< btDiscreteDynamicsWorld >             dynamicsWorld;

    std::shared_ptr< btCollisionShape >     groundShape;
    std::shared_ptr< btDefaultMotionState > groundMotionState;
    std::shared_ptr< btRigidBody >          groundRigidBody;

    std::shared_ptr< btCollisionShape >     fallShape;
    std::shared_ptr< btDefaultMotionState > fallMotionState;
    std::shared_ptr< btRigidBody >          fallRigidBody;
};

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
}

// -------------------------------------------------------------------------------------------------
bool Physics::initialize(Platform &platform)
{
    state = std::make_shared< State >();

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
            nullptr, state->groundShape.get(), btVector3(0, 0, 0));
        groundRigidBodyCI.m_restitution = 1.0;
        state->groundRigidBody = std::make_shared< btRigidBody >(groundRigidBodyCI);
    }

    {
        state->fallShape = std::make_shared< btSphereShape >(1.0f);

        btScalar fallMass = 1;
        btVector3 fallInertia(0, 0, 0);
        state->fallShape->calculateLocalInertia(fallMass, fallInertia);

        state->fallMotionState = std::make_shared< btDefaultMotionState >(
            btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 50)));
        btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(
            fallMass,
            state->fallMotionState.get(),
            state->fallShape.get(),
            fallInertia);
        fallRigidBodyCI.m_restitution = 0.5;
        state->fallRigidBody = std::make_shared< btRigidBody >(fallRigidBodyCI);
    }

    state->dynamicsWorld->addRigidBody(state->groundRigidBody.get());
    state->dynamicsWorld->addRigidBody(state->fallRigidBody.get());

    return true;
}

// -------------------------------------------------------------------------------------------------
void Physics::shutdown(Platform &platform)
{
    state->dynamicsWorld->removeRigidBody(state->groundRigidBody.get());
    state->dynamicsWorld->removeRigidBody(state->fallRigidBody.get());

    state = nullptr;
}

// -------------------------------------------------------------------------------------------------
void Physics::update(Platform &platform, double deltaTimeInS)
{
    state->dynamicsWorld->stepSimulation(btScalar(deltaTimeInS), 10);

    {
        Renderer::Mesh::Info *end, *first = platform.stateDb.fullState(
            Renderer::Mesh::Info::STATE, &end);
        if (end > first)
        {
            btTransform worldTrans;
            state->fallRigidBody->getMotionState()->getWorldTransform(worldTrans);
            memcpy(&first->position.x, worldTrans.getOrigin().m_floats, sizeof(first->position));
        }
    }
}
