// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#include "Physics.hpp"

#include <list>

#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include "Logging.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Platform.hpp"
#include "Renderer.hpp"

struct Physics::PrivateRigidBody
{
    // FIXME(martinmo): Member-variables of structs/classes with non-static methods
    // FIXME(martinmo): other than c'tor/d'tor should start with 'm_' prefix
    PrivateState &state;
    u64 handle = 0;

    std::shared_ptr< btDefaultMotionState > motionState;
    std::shared_ptr< btRigidBody > bulletRigidBody;

    std::vector< Affector::Info * > affectors;

    PrivateRigidBody(PrivateState &stateInit, RigidBody::Info *rigidBody);
    virtual ~PrivateRigidBody();
};

struct Physics::PrivateConstraint
{
    PrivateState &state;
    u64 handle = 0;

    std::shared_ptr< btTypedConstraint > bulletConstraint;

    PrivateConstraint(PrivateState &stateInit, Constraint::Info *constraint);
    virtual ~PrivateConstraint();
};

struct Physics::PrivateState
{
    std::shared_ptr< btBroadphaseInterface > broadphase;
    std::shared_ptr< btDefaultCollisionConfiguration > collisionConfiguration;
    std::shared_ptr< btCollisionDispatcher > dispatcher;

    std::shared_ptr< btSequentialImpulseConstraintSolver > solver;
    std::shared_ptr< btDiscreteDynamicsWorld > dynamicsWorld;

    std::shared_ptr< btCollisionShape > cubeShape;

    std::map< u64, btCollisionShape * > collisionShapes;
    std::list< std::shared_ptr< btCollisionShape > > collisionShapesStorage;

    std::list< std::shared_ptr< PrivateRigidBody > > privateRigidBodies;
    std::list< std::shared_ptr< PrivateConstraint > > privateConstraints;

    Platform &platform;

    static void preTickCallback(btDynamicsWorld *world, btScalar timeStep);

    PrivateState(Platform &platformInit);

    void preTick(btScalar timeStep);
};

struct Physics::RigidBody::PrivateInfo
{
    static u64 STATE;
    PrivateRigidBody *state = nullptr;
};

struct Physics::Constraint::PrivateInfo
{
    static u64 STATE;
    PrivateConstraint *state = nullptr;
};

u64 Physics::RigidBody::TYPE = 0;
u64 Physics::RigidBody::Info::STATE = 0;
u64 Physics::RigidBody::PrivateInfo::STATE = 0;

u64 Physics::Constraint::TYPE = 0;
u64 Physics::Constraint::Info::STATE = 0;
u64 Physics::Constraint::PrivateInfo::STATE = 0;

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
btQuaternion toBulletQuat(const glm::fquat &quat)
{
    return btQuaternion(quat.x, quat.y, quat.z, quat.w);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::PrivateRigidBody(
    PrivateState &stateInit, RigidBody::Info *rigidBody)
    : state(stateInit)
{
    btCollisionShape *collisionShape = nullptr;
    auto mesh = state.platform.stateDb.refState< Renderer::Mesh::Info >(rigidBody->meshHandle);
    u64 collisionShapeKey = u64(rigidBody->collisionShapeType) << 32 | u64(mesh->modelAsset);

    // TODO(martinmo): Find way to get rid of map lookup
    auto collisionShapeIter = state.collisionShapes.find(collisionShapeKey);
    if (collisionShapeIter == state.collisionShapes.end())
    {
        const Assets::Model *model = state.platform.assets.refModel(mesh->modelAsset);
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
        glm::fvec3 center = min + halfExtent;

        btTransform centerTransform = btTransform(
            btQuaternion(0.0f, 0.0f, 0.0f, 1.0f),
            btVector3(center.x, center.y, center.z));

        if (rigidBody->collisionShapeType == CollisionShapeType::BOUNDING_BOX)
        {
            collisionShape = new btBoxShape(btVector3(halfExtent.x, halfExtent.y, halfExtent.z));

            btCompoundShape *compoundShape = new btCompoundShape;
            compoundShape->addChildShape(centerTransform, collisionShape);
            collisionShape = compoundShape;
        }
        else if (rigidBody->collisionShapeType == CollisionShapeType::BOUNDING_SPHERE)
        {
            collisionShape = new btSphereShape(glm::max(
                glm::max(halfExtent.x, halfExtent.y), halfExtent.z));

            btCompoundShape *compoundShape = new btCompoundShape;
            compoundShape->addChildShape(centerTransform, collisionShape);
            collisionShape = compoundShape;
        }
        else if (rigidBody->collisionShapeType == CollisionShapeType::CONVEX_HULL_COMPOUND)
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
    if (glm::abs(mesh->rotation.x) + glm::abs(mesh->rotation.y)
      + glm::abs(mesh->rotation.z) + glm::abs(mesh->rotation.w) < 0.01f)
    {
        mesh->rotation = glm::fquat(1.0, 0.0, 0.0, 0.0);
    }
    motionState = std::make_shared< btDefaultMotionState >(
        btTransform(toBulletQuat(mesh->rotation), toBulletVec(mesh->translation)));

    btVector3 inertia(0, 0, 0);
    collisionShape->calculateLocalInertia(rigidBody->mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        rigidBody->mass, motionState.get(), collisionShape, inertia);
    constructionInfo.m_restitution = btScalar(0.25);
    constructionInfo.m_friction = btScalar(0.5);
    constructionInfo.m_rollingFriction = btScalar(0.2);
    bulletRigidBody = std::make_shared< btRigidBody >(constructionInfo);

    // TODO(martinmo): Add flag to disable this for specific RBs only
    bulletRigidBody->forceActivationState(DISABLE_DEACTIVATION);

    // Bullet collision groups/masks are 16 bit only
    COMMON_ASSERT(!(rigidBody->collisionGroup & 0xffff0000));
    COMMON_ASSERT(!(rigidBody->collisionMask  & 0xffff0000));

    state.dynamicsWorld->addRigidBody(bulletRigidBody.get(),
        short(rigidBody->collisionGroup), short(rigidBody->collisionMask));
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
Physics::PrivateConstraint::PrivateConstraint(
    PrivateState &stateInit, Constraint::Info *constraint)
    : state(stateInit)
{
    btRigidBody *rbA = state.platform.stateDb.refState< RigidBody::PrivateInfo >(
        constraint->rigidBodyAHandle)->state->bulletRigidBody.get();
    btRigidBody *rbB = state.platform.stateDb.refState< RigidBody::PrivateInfo >(
        constraint->rigidBodyBHandle)->state->bulletRigidBody.get();

    btTransform frameInA = btTransform(
        btQuaternion(
            constraint->paramQuatA.x, constraint->paramQuatA.y,
            constraint->paramQuatA.z, constraint->paramQuatA.w),
        btVector3(constraint->paramVecA.x, constraint->paramVecA.y, constraint->paramVecA.z));
    btTransform frameInB = btTransform(
        btQuaternion(
            constraint->paramQuatB.x, constraint->paramQuatB.y,
            constraint->paramQuatB.z, constraint->paramQuatB.w),
        btVector3(constraint->paramVecB.x, constraint->paramVecB.y, constraint->paramVecB.z));

    bulletConstraint = std::make_shared< btFixedConstraint >(*rbA, *rbB, frameInA, frameInB);
    state.dynamicsWorld->addConstraint(bulletConstraint.get(), true);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateConstraint::~PrivateConstraint()
{
    if (bulletConstraint)
    {
        state.dynamicsWorld->removeConstraint(bulletConstraint.get());
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    PrivateState *state = (PrivateState *)world->getWorldUserInfo();
    state->preTick(timeStep);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateState::PrivateState(Platform &platformInit) :
    platform(platformInit)
{
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTick(btScalar timeStep)
{
    for (const std::shared_ptr< PrivateRigidBody > &privateRigidBody : privateRigidBodies)
    {
        btRigidBody *bulletRigidBody = privateRigidBody->bulletRigidBody.get();
        bulletRigidBody->clearForces();
        bulletRigidBody->applyGravity();
        for (const Affector::Info *affector : privateRigidBody->affectors)
        {
            glm::fquat rotation = fromBulletQuat(
                bulletRigidBody->getCenterOfMassTransform().getRotation());
            glm::fvec3 forcePositionGlobal = rotation * affector->forcePosition;
            bulletRigidBody->applyForce(
                toBulletVec(affector->force), toBulletVec(forcePositionGlobal));
            bulletRigidBody->applyTorque(toBulletVec(affector->torque));
        }
    }

    // Apply linear velocity limits to all RBs
    RigidBody::Info *rigidBody = nullptr, *rigidBodyBegin = nullptr, *rigidBodyEnd = nullptr;
    platform.stateDb.refStateAll(&rigidBodyBegin, &rigidBodyEnd);
    RigidBody::PrivateInfo *rigidBodyPrivate = nullptr, *rigidBodyPrivateBegin = nullptr;
    platform.stateDb.refStateAll(&rigidBodyPrivateBegin);
    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        btRigidBody *bulletRigidBody = rigidBodyPrivate->state->bulletRigidBody.get();
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

    Affector::TYPE = stateDb.registerType("Affector", 512);
    Affector::Info::STATE = stateDb.registerState(
        Affector::TYPE, "Info", sizeof(Affector::Info));

    Constraint::TYPE = stateDb.registerType("Constraint", 512);
    Constraint::Info::STATE = stateDb.registerState(
        Constraint::TYPE, "Info", sizeof(Constraint::Info));
    Constraint::PrivateInfo::STATE = stateDb.registerState(
        Constraint::TYPE, "PrivateInfo", sizeof(Constraint::PrivateInfo));
}

// -------------------------------------------------------------------------------------------------
bool Physics::initialize(Platform &platform)
{
    state = std::make_shared< PrivateState >(platform);

    state->collisionConfiguration = std::make_shared< btDefaultCollisionConfiguration >();
    state->dispatcher = std::make_shared< btCollisionDispatcher >(
        state->collisionConfiguration.get());

    state->broadphase = std::make_shared< btDbvtBroadphase >();
    state->solver = std::make_shared< btSequentialImpulseConstraintSolver >();

    state->dynamicsWorld = std::make_shared< btDiscreteDynamicsWorld >(
        state->dispatcher.get(), state->broadphase.get(),
        state->solver.get(), state->collisionConfiguration.get());

    state->dynamicsWorld->setGravity(btVector3(0, 0, -10));

    /*
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
    */

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
    //state->dynamicsWorld->removeRigidBody(state->groundRigidBody.get());
    state = nullptr;
}

// -------------------------------------------------------------------------------------------------
template< class SrcType, class DstType, class UserData >
void trackCreations(StateDb &stateDb,
    std::list< std::shared_ptr< DstType > > &dstStorage, UserData &userData)
{
    typename SrcType::Info *srcBegin = nullptr, *srcEnd = nullptr, *src = nullptr;
    stateDb.refStateAll(&srcBegin, &srcEnd);
    typename SrcType::PrivateInfo *srcPrivateBegin = nullptr, *srcPrivate = nullptr;
    stateDb.refStateAll(&srcPrivateBegin);
    for (src  = srcBegin, srcPrivate = srcPrivateBegin;
         src != srcEnd; ++src, ++srcPrivate)
    {
        if (!srcPrivate->state)
        {
            dstStorage.push_back(std::make_shared< DstType >(userData, src));
            srcPrivate->state = dstStorage.back().get();
            srcPrivate->state->handle = stateDb.objectHandleFromElem(src);
            Logging::debug("Creation of \"%s\" (%d instances tracked)",
                stateDb.objectHandleTypeName(srcPrivate->state->handle).c_str(),
                int(dstStorage.size()));
        }
    }
}

// -------------------------------------------------------------------------------------------------
template< class DstType >
void trackDestructions(StateDb &stateDb,
    std::list< std::shared_ptr< DstType > > &dstStorage)
{
    typename std::list< std::shared_ptr< DstType > >::iterator dstStorageIter;
    std::vector< decltype(dstStorageIter) > deletions;
    for (dstStorageIter  = dstStorage.begin();
         dstStorageIter != dstStorage.end(); ++dstStorageIter)
    {
        if (!stateDb.isObjectHandleValid(dstStorageIter->get()->handle))
        {
            deletions.push_back(dstStorageIter);
        }
    }
    while (!deletions.empty())
    {
        dstStorage.erase(deletions.back());
        deletions.pop_back();
        Logging::debug("Destruction of \"%s\" (%d instances tracked)",
            stateDb.objectHandleTypeName(dstStorageIter->get()->handle).c_str(),
            int(dstStorage.size()));
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::update(Platform &platform, double deltaTimeInS)
{
    RigidBody::Info *rigidBody = nullptr, *rigidBodyBegin = nullptr, *rigidBodyEnd = nullptr;
    platform.stateDb.refStateAll(&rigidBodyBegin, &rigidBodyEnd);
    RigidBody::PrivateInfo *rigidBodyPrivate = nullptr, *rigidBodyPrivateBegin = nullptr;
    platform.stateDb.refStateAll(&rigidBodyPrivateBegin);

    // Check for newly created rigid bodies
    trackCreations< RigidBody >(platform.stateDb, state->privateRigidBodies, *state.get());
    // Check for newly created constraints
    trackCreations< Constraint >(platform.stateDb, state->privateConstraints, *state.get());

    // Clear old and find new affectors of rigid bodies
    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        rigidBodyPrivate->state->affectors.clear();
    }
    Affector::Info *affector = nullptr, *affectorBegin = nullptr, *affectorEnd = nullptr;
    platform.stateDb.refStateAll(&affectorBegin, &affectorEnd);
    for (affector = affectorBegin; affector != affectorEnd; ++affector)
    {
        if (affector->enabled)
        {
            rigidBodyPrivate = platform.stateDb.refState<
                RigidBody::PrivateInfo >(affector->rigidBodyHandle);
            rigidBodyPrivate->state->affectors.push_back(affector);
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

    // Update mesh transforms according to rigid bodies
    for (rigidBody  = rigidBodyBegin, rigidBodyPrivate = rigidBodyPrivateBegin;
         rigidBody != rigidBodyEnd; ++rigidBody, ++rigidBodyPrivate)
    {
        // TODO(martinmo): Delete rigid bodies with bad mesh references?
        // TODO(martinmo): (e.g. mesh has been destroyed but rigid body still alive)
        if (!platform.stateDb.isObjectHandleValid(rigidBody->meshHandle))
        {
            continue;
        }

        btRigidBody *bulletRigidBody = rigidBodyPrivate->state->bulletRigidBody.get();
        const btTransform &worldTrans = bulletRigidBody->getCenterOfMassTransform();

        rigidBody->linearVelocity  = fromBulletVec(bulletRigidBody->getLinearVelocity());
        rigidBody->angularVelocity = fromBulletVec(bulletRigidBody->getAngularVelocity());

        auto mesh = platform.stateDb.refState< Renderer::Mesh::Info >(rigidBody->meshHandle);

        mesh->translation = fromBulletVec(worldTrans.getOrigin());
        mesh->rotation    = fromBulletQuat(worldTrans.getRotation());
    }

    // Check for destroyed rigid bodies
    trackDestructions(platform.stateDb, state->privateRigidBodies);
    // Check for destroyed constraints
    trackDestructions(platform.stateDb, state->privateConstraints);
}
