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
    // FIXME(martinmo): Member-variables of structs/classes with non-static methods
    // FIXME(martinmo): other than c'tor/d'tor should start with 'm_' prefix

    PrivateState &state;

    u64 objectHandle = 0;

    std::shared_ptr< btDefaultMotionState > motionState;
    std::shared_ptr< btRigidBody > bulletRigidBody;

    std::vector< Affector::Info * > affectors;

    PrivateRigidBody(PrivateState &state);
    virtual ~PrivateRigidBody();

    void initialize(u64 objectHandle, Physics::RigidBody::Info *rigidBody,
        Renderer::Mesh::Info *mesh, const Assets::Model *model);
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

    std::map< u64, btCollisionShape * > collisionShapes;
    std::list< std::shared_ptr< btCollisionShape > > collisionShapesStorage;

    std::list< PrivateRigidBody > privateRigidBodies;

    StateDb &stateDb;

    static void preTickCallback(btDynamicsWorld *world, btScalar timeStep);

    PrivateState(StateDb &stateDbInit);

    void preTick(btScalar timeStep);
};

struct Physics::RigidBody::PrivateInfo
{
    static u64 STATE;
    PrivateRigidBody *privateRigidBody = nullptr;
};

u64 Physics::RigidBody::TYPE = 0;
u64 Physics::RigidBody::Info::STATE = 0;
u64 Physics::RigidBody::PrivateInfo::STATE = 0;

u64 Physics::Affector::TYPE = 0;
u64 Physics::Affector::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
glm::fvec3 fromBulletVec(const btVector3 &bulletVec)
{
    return glm::fvec3(bulletVec.x(), bulletVec.y(), bulletVec.z());
}

// -------------------------------------------------------------------------------------------------
btVector3 toBulletVec(const glm::fvec3 &vec)
{
    return btVector3(vec.x, vec.y, vec.z);
}

// -------------------------------------------------------------------------------------------------
glm::quat fromBulletQuat(const btQuaternion &bulletQuat)
{
    return glm::fquat(bulletQuat.w(), bulletQuat.x(), bulletQuat.y(), bulletQuat.z());
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::PrivateRigidBody(PrivateState &state) :
    state(state)
{
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::~PrivateRigidBody()
{
    if (bulletRigidBody)
    {
        state.dynamicsWorld->removeRigidBody(bulletRigidBody.get());
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateRigidBody::initialize(
    u64 objectHandle, Physics::RigidBody::Info *rigidBody,
    Renderer::Mesh::Info *mesh, const Assets::Model *model)
{
    this->objectHandle = objectHandle;

    btCollisionShape *collisionShape = nullptr;
    u64 collisionShapeKey = u64(rigidBody->collisionShapeType) << 32 | u64(mesh->modelAsset);

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

        if (rigidBody->collisionShapeType == Physics::RigidBody::Info::BOUNDING_BOX)
        {
            collisionShape = new btBoxShape(btVector3(halfExtent.x, halfExtent.y, halfExtent.z));
        }
        else if (rigidBody->collisionShapeType == Physics::RigidBody::Info::BOUNDING_SPHERE)
        {
            collisionShape = new btSphereShape(glm::max(
                glm::max(halfExtent.x, halfExtent.y), halfExtent.z));
        }
        else if (rigidBody->collisionShapeType == Physics::RigidBody::Info::CONVEX_HULL_COMPOUND)
        {
            btCompoundShape *compoundShape = new btCompoundShape;
            for (auto &subMesh : model->subMeshes)
            {
                /*
                btConvexHullShape *shape = new btConvexHullShape(
                    &model->positions[subMesh.triangleOffset * 3].x, subMesh.triangleCount * 3);
                shape->recalcLocalAabb();
                */

                btConvexHullShape *shape = new btConvexHullShape;
                const glm::fvec3 *positionIter = &model->positions[subMesh.triangleOffset * 3];
                const u64 pointCount = subMesh.triangleCount * 3;
                for (u64 pointIdx = 0; pointIdx < pointCount; ++pointIdx)
                {
                    btVector3 point = btVector3(positionIter->x, positionIter->y, positionIter->z);
                    shape->addPoint(point);
                    ++positionIter;
                }

                // TODO(martinmo): Optimize convex hull shape using 'btShapeHull'
                btTransform transform = btTransform(btQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
                compoundShape->addChildShape(transform, shape);
            }
            collisionShape = compoundShape;
        }
        else
        {
            // TODO(martinmo): Collision shape fallback warning?
            collisionShape = state.cubeShape.get();
        }
        if (collisionShape != state.cubeShape.get())
        {
            state.collisionShapesStorage.push_back(
                std::shared_ptr< btCollisionShape >(collisionShape));
        }
        state.collisionShapes[collisionShapeKey] = collisionShape;
    }
    else
    {
        collisionShape = collisionShapeIter->second;
    }

    // TODO(martinmo): Update collision shape if asset version changes

    // Make sure initial rotation is a sane value
    if (glm::abs(mesh->rotation.x) + glm::abs(mesh->rotation.x)
        + glm::abs(mesh->rotation.x) + glm::abs(mesh->rotation.x) < 0.01f)
    {
        mesh->rotation = glm::fquat(1.0, 0.0, 0.0, 0.0);
    }
    btQuaternion rotation(
        mesh->rotation.x, mesh->rotation.y,
        mesh->rotation.z, mesh->rotation.w);
    btVector3 translation(
        mesh->translation.x, mesh->translation.y, mesh->translation.z);
    motionState = std::make_shared< btDefaultMotionState >(btTransform(rotation, translation));

    btScalar mass = 5;
    btVector3 inertia(0, 0, 0);
    collisionShape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass, motionState.get(), collisionShape, inertia);
    constructionInfo.m_restitution = btScalar(0.25);
    constructionInfo.m_friction = btScalar(0.5);
    constructionInfo.m_rollingFriction = btScalar(0.2);
    bulletRigidBody = std::make_shared< btRigidBody >(constructionInfo);

    // TODO(martinmo): Add flag to disable this for specific RBs only
    bulletRigidBody->forceActivationState(DISABLE_DEACTIVATION);

    // Bullet collision groups/masks are 16 bit only
    COMMON_ASSERT(!(rigidBody->collisionGroup & 0xffff0000));
    COMMON_ASSERT(!(rigidBody->collisionMask & 0xffff0000));

    state.dynamicsWorld->addRigidBody(bulletRigidBody.get(),
        short(rigidBody->collisionGroup), short(rigidBody->collisionMask));
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    PrivateState *state = (PrivateState *)world->getWorldUserInfo();
    state->preTick(timeStep);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateState::PrivateState(StateDb &stateDbInit) :
    stateDb(stateDbInit)
{
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTick(btScalar timeStep)
{
    for (auto &privateRigidBody : privateRigidBodies)
    {
        btRigidBody *bulletRigidBody = privateRigidBody.bulletRigidBody.get();
        bulletRigidBody->clearForces();
        bulletRigidBody->applyGravity();
        for (const Affector::Info *affector : privateRigidBody.affectors)
        {
            bulletRigidBody->applyForce(
                toBulletVec(affector->force), toBulletVec(affector->forcePosition));
            bulletRigidBody->applyTorque(toBulletVec(affector->torque));
        }
    }

    // Apply linear velocity limits to all RBs
    RigidBody::Info *rigidBody = nullptr, *rigidBodyBegin = nullptr, *rigidBodyEnd = nullptr;
    stateDb.refStateAll(&rigidBodyBegin, &rigidBodyEnd);
    RigidBody::PrivateInfo *rigidBodyPrivate = nullptr, *rigidBodyPrivateBegin = nullptr;
    stateDb.refStateAll(&rigidBodyPrivateBegin);
    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        btRigidBody *bulletRigidBody = rigidBodyPrivate->privateRigidBody->bulletRigidBody.get();
        btVector3 linearVelocity = bulletRigidBody->getLinearVelocity();
        glm::fvec3 limit = rigidBody->linearVelocityLimit;
        if (limit.x > 0.0f)
        {
            if (linearVelocity.x() >  limit.x) linearVelocity.setX( limit.x);
            if (linearVelocity.x() < -limit.x) linearVelocity.setX(-limit.x);
        }
        if (limit.y > 0.0f)
        {
            if (linearVelocity.y() >  limit.y) linearVelocity.setY( limit.y);
            if (linearVelocity.y() < -limit.y) linearVelocity.setY(-limit.y);
        }
        if (limit.z > 0.0f)
        {
            if (linearVelocity.z() >  limit.z)
            {
                linearVelocity.setZ( limit.z);
            }
            if (linearVelocity.z() < -limit.z)
            {
                linearVelocity.setZ(-limit.z);
            }
        }
        bulletRigidBody->setLinearVelocity(linearVelocity);
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
    RigidBody::TYPE = stateDb.registerType("RigidBody", 4096);
    RigidBody::Info::STATE = stateDb.registerState(
        RigidBody::TYPE, "Info", sizeof(RigidBody::Info));
    RigidBody::PrivateInfo::STATE = stateDb.registerState(
        RigidBody::TYPE, "PrivateInfo", sizeof(RigidBody::PrivateInfo));

    Affector::TYPE = stateDb.registerType("Affector", 256);
    Affector::Info::STATE = stateDb.registerState(
        Affector::TYPE, "Info", sizeof(Affector::Info));
}

// -------------------------------------------------------------------------------------------------
bool Physics::initialize(Platform &platform)
{
    state = std::make_shared< PrivateState >(platform.stateDb);

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
        state->dynamicsWorld->addRigidBody(state->groundRigidBody.get(), 1, 0xffff);
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
    RigidBody::Info *rigidBody = nullptr, *rigidBodyBegin = nullptr, *rigidBodyEnd = nullptr;
    platform.stateDb.refStateAll(&rigidBodyBegin, &rigidBodyEnd);
    RigidBody::PrivateInfo *rigidBodyPrivate = nullptr, *rigidBodyPrivateBegin = nullptr;
    platform.stateDb.refStateAll(&rigidBodyPrivateBegin);

    // Check for newly created rigid bodies
    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        auto mesh = platform.stateDb.refState< Renderer::Mesh::Info >(rigidBody->meshHandle);
        if (!rigidBodyPrivate->privateRigidBody)
        {
            u64 objectHandle = platform.stateDb.objectHandleFromElem(rigidBody);
            state->privateRigidBodies.push_back(PrivateRigidBody(*state.get()));
            PrivateRigidBody *privateRigidBody = &state->privateRigidBodies.back();

            const Assets::Model *model = platform.assets.refModel(mesh->modelAsset);
            privateRigidBody->initialize(objectHandle, rigidBody, mesh, model);

            rigidBodyPrivate->privateRigidBody = privateRigidBody;
        }
        rigidBodyPrivate->privateRigidBody->affectors.clear();
    }

    Affector::Info *affector = nullptr, *affectorBegin = nullptr, *affectorEnd = nullptr;
    platform.stateDb.refStateAll(&affectorBegin, &affectorEnd);
    for (affector = affectorBegin; affector != affectorEnd; ++affector)
    {
        if (affector->enabled)
        {
            rigidBodyPrivate = platform.stateDb.refState<
                RigidBody::PrivateInfo >(affector->rigidBodyHandle);
            rigidBodyPrivate->privateRigidBody->affectors.push_back(affector);
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

    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        // TODO(martinmo): Delete rigid bodies with bad mesh references?
        // TODO(martinmo): (e.g. mesh has been destroyed and rigid body still alive)
        if (!platform.stateDb.isObjectHandleValid(rigidBody->meshHandle))
        {
            continue;
        }

        btRigidBody *bulletRigidBody = rigidBodyPrivate->privateRigidBody->bulletRigidBody.get();
        const btTransform &worldTrans = bulletRigidBody->getCenterOfMassTransform();

        rigidBody->linearVelocity = fromBulletVec(bulletRigidBody->getLinearVelocity());
        rigidBody->angularVelocity = fromBulletVec(bulletRigidBody->getAngularVelocity());

        auto mesh = platform.stateDb.refState< Renderer::Mesh::Info >(rigidBody->meshHandle);

        mesh->translation = fromBulletVec(worldTrans.getOrigin());
        mesh->rotation = fromBulletQuat(worldTrans.getRotation());
    }

    // Check for destroyed rigid bodies
    std::list< PrivateRigidBody >::iterator rigidBodiesIter;
    std::vector< decltype(rigidBodiesIter) > deletions;
    for (rigidBodiesIter = state->privateRigidBodies.begin();
         rigidBodiesIter != state->privateRigidBodies.end();
         ++rigidBodiesIter)
    {
        if (!platform.stateDb.isObjectHandleValid(rigidBodiesIter->objectHandle))
        {
            deletions.push_back(rigidBodiesIter);
        }
    }
    while (!deletions.empty())
    {
        state->privateRigidBodies.erase(deletions.back());
        deletions.pop_back();
    }
}
