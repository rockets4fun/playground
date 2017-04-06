// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#include "Physics.hpp"

#include <list>

#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include "Logger.hpp"
#include "Profiler.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"

struct Physics::PrivateRigidBody
{
    PrivateState &m_state;

    u64 handle = 0;

    std::shared_ptr< btDefaultMotionState > motionState;
    std::shared_ptr< btRigidBody > bulletRigidBody;

    std::vector< Affector::Info * > affectors;

    PrivateRigidBody(PrivateState &stateInit, RigidBody::Info *rigidBody);
    virtual ~PrivateRigidBody();
};

struct Physics::PrivateConstraint
{
    PrivateState &m_state;
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

    StateDb &sdb;
    Assets &assets;

    static void preTickCallback(btDynamicsWorld *world, btScalar timeStep);
    static void postTickCallback(btDynamicsWorld *world, btScalar timeStep);

    PrivateState(StateDb &sdbInit, Assets &assetsInit);

    void preTick(btScalar timeStep);
    void postTick(btScalar timeStep);
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

u64 Physics::World::TYPE = 0;
u64 Physics::World::Info::STATE = 0;

u64 Physics::RigidBody::TYPE = 0;
u64 Physics::RigidBody::Info::STATE = 0;
u64 Physics::RigidBody::PrivateInfo::STATE = 0;

u64 Physics::Constraint::TYPE = 0;
u64 Physics::Constraint::Info::STATE = 0;
u64 Physics::Constraint::PrivateInfo::STATE = 0;

u64 Physics::Affector::TYPE = 0;
u64 Physics::Affector::Info::STATE = 0;

u64 Physics::Sensor::TYPE = 0;
u64 Physics::Sensor::Info::STATE = 0;

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
    : m_state(stateInit)
{
    btCollisionShape *collisionShape = nullptr;
    auto mesh = m_state.sdb.state< Renderer::Mesh::Info >(rigidBody->meshHandle);
    u64 collisionShapeKey = u64(rigidBody->collisionShape) << 32 | u64(mesh->modelAsset);

    // TODO(martinmo): Find way to get rid of map lookup
    auto collisionShapeIter = m_state.collisionShapes.find(collisionShapeKey);
    if (collisionShapeIter == m_state.collisionShapes.end())
    {
        const Assets::Model *model = m_state.assets.refModel(mesh->modelAsset);
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

        if (rigidBody->collisionShape == RigidBody::CollisionShape::BOUNDING_BOX)
        {
            collisionShape = new btBoxShape(btVector3(halfExtent.x, halfExtent.y, halfExtent.z));

            btCompoundShape *compoundShape = new btCompoundShape;
            compoundShape->addChildShape(centerTransform, collisionShape);
            collisionShape = compoundShape;
        }
        else if (rigidBody->collisionShape == RigidBody::CollisionShape::BOUNDING_SPHERE)
        {
            collisionShape = new btSphereShape(glm::max(
                glm::max(halfExtent.x, halfExtent.y), halfExtent.z));

            btCompoundShape *compoundShape = new btCompoundShape;
            compoundShape->addChildShape(centerTransform, collisionShape);
            collisionShape = compoundShape;
        }
        else if (rigidBody->collisionShape == RigidBody::CollisionShape::CONVEX_HULL_COMPOUND)
        {
            btCompoundShape *compoundShape = new btCompoundShape;
            for (const auto &part : model->parts)
            {
                /*
                btConvexHullShape *shape = new btConvexHullShape(
                    &model->positions[subMesh.vertexOffset * 3].x, subMesh.triangleCount * 3);
                shape->recalcLocalAabb();
                */

                btConvexHullShape *shape = new btConvexHullShape;
                const glm::fvec3 *positionIter = &model->positions[part.offset];
                const u64 pointCount = part.count;
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
            Logger::debug("WARNING: Debug collision shape fallback");
            collisionShape = m_state.cubeShape.get();
        }
        if (collisionShape != m_state.cubeShape.get())
        {
            m_state.collisionShapesStorage.push_back(
                std::shared_ptr< btCollisionShape >(collisionShape));
        }
        m_state.collisionShapes[collisionShapeKey] = collisionShape;
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

    m_state.dynamicsWorld->addRigidBody(bulletRigidBody.get(),
        short(rigidBody->collisionGroup), short(rigidBody->collisionMask));
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateRigidBody::~PrivateRigidBody()
{
    if (bulletRigidBody)
    {
        m_state.dynamicsWorld->removeRigidBody(bulletRigidBody.get());
    }
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateConstraint::PrivateConstraint(
    PrivateState &stateInit, Constraint::Info *constraint)
    : m_state(stateInit)
{
    btRigidBody *rbA = m_state.sdb.state< RigidBody::PrivateInfo >(
        constraint->rigidBodyAHandle)->state->bulletRigidBody.get();
    btRigidBody *rbB = m_state.sdb.state< RigidBody::PrivateInfo >(
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
    m_state.dynamicsWorld->addConstraint(bulletConstraint.get(), true);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateConstraint::~PrivateConstraint()
{
    if (bulletConstraint)
    {
        m_state.dynamicsWorld->removeConstraint(bulletConstraint.get());
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::preTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    PrivateState *state = (PrivateState *)world->getWorldUserInfo();
    state->preTick(timeStep);
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::postTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    PrivateState *state = (PrivateState *)world->getWorldUserInfo();
    state->postTick(timeStep);
}

// -------------------------------------------------------------------------------------------------
Physics::PrivateState::PrivateState(StateDb &sdbInit, Assets &assetsInit) :
    sdb(sdbInit),
    assets(assetsInit)
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
}

// -------------------------------------------------------------------------------------------------
void Physics::PrivateState::postTick(btScalar timeStep)
{
    auto sensors = sdb.stateAll< Sensor::Info >();

    // Update rigid body sensors
    for (auto sensor : sensors)
    {
        auto rigidBodyPrivate = sdb.state< RigidBody::PrivateInfo >(sensor->simRbHandle);

        // FIXME(MARTINMO): Avoid pointer chase by introducing state holding raw pointer
        // FIXME(MARTINMO): => Maybe just add copy of pointer in 'RigidBody::PrivateInfo'
        const btRigidBody *bulletRigidBody = rigidBodyPrivate->state->bulletRigidBody.get();

        // FIXME(MARTINMO): Make sure we keep sensor readings w/ highest magnitude
        // FIXME(MARTINMO): throughout all internal ticks of a frame?

        // Debugging output
        {
            const btTransform &worldTrans = bulletRigidBody->getCenterOfMassTransform();
            glm::fquat rotation = fromBulletQuat(worldTrans.getRotation());

            glm::fvec3 force  = fromBulletVec(bulletRigidBody->getTotalForce());
            glm::fvec3 torque = fromBulletVec(bulletRigidBody->getTotalTorque());

            // Transform torque and force from global into RB local coordinate frame
            torque = torque * rotation;
            force  = force  * rotation;

            Logger::debug("%08X   force = %5.2f %5.2f %5.2f   torque = %5.2f %5.2f %5.2f",
                bulletRigidBody, force.x, force.y, force.z, torque.x, torque.y, torque.z);
        }
    }

    auto rigidBodies = sdb.stateAll< RigidBody::Info >();
    auto rigidBodiesPrivate = sdb.stateAll< RigidBody::PrivateInfo >();

    // Apply linear velocity limits to all RBs
    for (auto rigidBody : rigidBodies)
    {
        auto rigidBodyPrivate = rigidBodiesPrivate.rel(rigidBodies, rigidBody);
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
            if (linearVelocity.z() >  limit.z) linearVelocity.setZ( limit.z);
            if (linearVelocity.z() < -limit.z) linearVelocity.setZ(-limit.z);
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
u64 Physics::worldHandle() const
{
    return m_worldHandle;
}

// -------------------------------------------------------------------------------------------------
void Physics::registerTypesAndStates(StateDb &sdb)
{
    World::TYPE = sdb.registerType("World", 4096);
    World::Info::STATE = sdb.registerState(
        World::TYPE, "Info", sizeof(World::Info));

    RigidBody::TYPE = sdb.registerType("RigidBody", 4096);
    RigidBody::Info::STATE = sdb.registerState(
        RigidBody::TYPE, "Info", sizeof(RigidBody::Info));
    RigidBody::PrivateInfo::STATE = sdb.registerState(
        RigidBody::TYPE, "PrivateInfo", sizeof(RigidBody::PrivateInfo));

    Constraint::TYPE = sdb.registerType("Constraint", 512);
    Constraint::Info::STATE = sdb.registerState(
        Constraint::TYPE, "Info", sizeof(Constraint::Info));
    Constraint::PrivateInfo::STATE = sdb.registerState(
        Constraint::TYPE, "PrivateInfo", sizeof(Constraint::PrivateInfo));

    Affector::TYPE = sdb.registerType("Affector", 512);
    Affector::Info::STATE = sdb.registerState(
        Affector::TYPE, "Info", sizeof(Affector::Info));

    Sensor::TYPE = sdb.registerType("Sensor", 512);
    Sensor::Info::STATE = sdb.registerState(
        Sensor::TYPE, "Info", sizeof(Sensor::Info));
}

// -------------------------------------------------------------------------------------------------
bool Physics::initialize(StateDb &sdb, Assets &assets)
{
    m_state = std::make_shared< PrivateState >(sdb, assets);

    m_state->collisionConfiguration = std::make_shared< btDefaultCollisionConfiguration >();
    m_state->dispatcher = std::make_shared< btCollisionDispatcher >(
        m_state->collisionConfiguration.get());

    m_state->broadphase = std::make_shared< btDbvtBroadphase >();
    m_state->solver = std::make_shared< btSequentialImpulseConstraintSolver >();

    m_state->dynamicsWorld = std::make_shared< btDiscreteDynamicsWorld >(
        m_state->dispatcher.get(), m_state->broadphase.get(),
        m_state->solver.get(), m_state->collisionConfiguration.get());

    /*
    {
        m_state->groundShape = std::make_shared< btStaticPlaneShape >(btVector3(0, 0, 1), 0.0f);
        m_state->groundMotionState = std::make_shared< btDefaultMotionState> (
            btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0,
            m_state->groundMotionState.get(), m_state->groundShape.get());
        groundRigidBodyCI.m_restitution = btScalar(1.0);
        groundRigidBodyCI.m_friction = btScalar(1.0);
        groundRigidBodyCI.m_rollingFriction = btScalar(1.0);
        m_state->groundRigidBody = std::make_shared< btRigidBody >(groundRigidBodyCI);
        m_state->dynamicsWorld->addRigidBody(m_state->groundRigidBody.get(), 1, 0xffff);
    }
    */

    {
        m_state->cubeShape = std::make_shared< btBoxShape >(btVector3(0.5, 0.5, 0.5));
    }

    //m_state->dynamicsWorld->setSynchronizeAllMotionStates(...)
    m_state->dynamicsWorld->setInternalTickCallback(
        PrivateState::preTickCallback, m_state.get(), true);
    m_state->dynamicsWorld->setInternalTickCallback(
        PrivateState::postTickCallback, m_state.get(), false);

    auto world = sdb.create< World::Info >(m_worldHandle);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Physics::shutdown(StateDb &sdb)
{
    m_state->dynamicsWorld->setInternalTickCallback(nullptr);
    //m_state->dynamicsWorld->removeRigidBody(m_state->groundRigidBody.get());
    m_state = nullptr;
}

// -------------------------------------------------------------------------------------------------
template< class SrcType, class DstType, class UserData >
void trackCreations(StateDb &sdb,
    std::list< std::shared_ptr< DstType > > &dstStorage, UserData &userData)
{
    auto srcRange = sdb.stateAll< typename SrcType::Info >();
    auto srcPrivateRange = sdb.stateAll< typename SrcType::PrivateInfo >();
    for (auto srcPrivate : srcPrivateRange)
    {
        if (!srcPrivate->state)
        {
            auto src = srcRange.rel(srcPrivateRange, srcPrivate);
            dstStorage.push_back(std::make_shared< DstType >(userData, src));
            srcPrivate->state = dstStorage.back().get();
            srcPrivate->state->handle = sdb.handleFromState(src);
            Logger::debug("Creation of \"%s\" (%d instances tracked)",
                sdb.handleTypeName(srcPrivate->state->handle).c_str(),
                int(dstStorage.size()));
        }
    }
}

// -------------------------------------------------------------------------------------------------
template< class DstType >
void trackDestructions(StateDb &sdb,
    std::list< std::shared_ptr< DstType > > &dstStorage)
{
    typename std::list< std::shared_ptr< DstType > >::iterator dstStorageIter;
    std::vector< decltype(dstStorageIter) > deletions;
    for (dstStorageIter  = dstStorage.begin();
         dstStorageIter != dstStorage.end(); ++dstStorageIter)
    {
        if (!sdb.isHandleValid(dstStorageIter->get()->handle))
        {
            deletions.push_back(dstStorageIter);
        }
    }
    while (!deletions.empty())
    {
        dstStorage.erase(deletions.back());
        deletions.pop_back();
        Logger::debug("Destruction of \"%s\" (%d instances tracked)",
            sdb.handleTypeName(dstStorageIter->get()->handle).c_str(),
            int(dstStorage.size()));
    }
}

// -------------------------------------------------------------------------------------------------
void Physics::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    PROFILER_SECTION(Physics, glm::fvec3(1.0f, 0.0f, 0.0f))

    // Set world gravity from state
    auto world = sdb.state< World::Info >(m_worldHandle);
    m_state->dynamicsWorld->setGravity(toBulletVec(world->gravity));

    // Check for newly created rigid bodies
    trackCreations< RigidBody >(sdb, m_state->privateRigidBodies, *m_state.get());
    // Check for newly created constraints
    trackCreations< Constraint >(sdb, m_state->privateConstraints, *m_state.get());

    auto rigidBodies = sdb.stateAll< RigidBody::Info >();
    auto rigidBodiesPrivate = sdb.stateAll< RigidBody::PrivateInfo >();

    // Update rigid body affectors
    for (auto rigidBodyPrivate : rigidBodiesPrivate)
    {
        rigidBodyPrivate->state->affectors.clear();
    }
    auto affectors = sdb.stateAll< Affector::Info >();
    for (auto affector : affectors)
    {
        if (affector->enabled)
        {
            auto rigidBodyPrivate = sdb.state< RigidBody::PrivateInfo >(affector->rigidBodyHandle);
            rigidBodyPrivate->state->affectors.push_back(affector);
        }
    }

    // Handle mesh => rigid body state forwarding
    for (auto rigidBody : rigidBodies)
    {
        // TODO(martinmo): Delete rigid bodies with bad mesh references?
        // TODO(martinmo): (e.g. mesh has been destroyed but rigid body still alive)
        if (!sdb.isHandleValid(rigidBody->meshHandle))
        {
            continue;
        }

        if (!(rigidBody->flags & RigidBody::Flag::RESET_TO_MESH))
        {
            continue;
        }

        auto rigidBodyPrivate = rigidBodiesPrivate.rel(rigidBodies, rigidBody);
        auto mesh = sdb.state< Renderer::Mesh::Info >(rigidBody->meshHandle);

        btRigidBody *bulletRigidBody = rigidBodyPrivate->state->bulletRigidBody.get();

        bulletRigidBody->setLinearVelocity(btVector3(0, 0, 0));
        bulletRigidBody->setAngularVelocity(btVector3(0, 0, 0));
        bulletRigidBody->setWorldTransform(
            btTransform(toBulletQuat(mesh->rotation), toBulletVec(mesh->translation)));

        rigidBody->flags &= ~RigidBody::Flag::RESET_TO_MESH;
    }

    // FIXME(MARTINMO): Reset all sensor readings to 0-magnitude before
    // FIXME(MARTINMO): executing the first internal tick
    // FIXME(MARTINMO): => What if no internal tick is executed subsequently?
    // FIXME(MARTINMO): => (application logic would operate on 0-magnitude readings)

    // Update rigid body physics simulation
    {
        PROFILER_SECTION(StepSim, glm::fvec3(1.0f, 0.5f, 0.0f))
        // FIXME(MARTINMO): Make sure to always perform one fixed internal step?
        m_state->dynamicsWorld->stepSimulation(btScalar(deltaTimeInS), 5, btScalar(1.0 / 60));
    }

    // TODO(martinmo): Use 'CProfileIterator' to present profiling data in meaningful way
    /*
#ifdef COMMON_DEBUG
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
    for (auto rigidBody : rigidBodies)
    {
        // TODO(martinmo): Delete rigid bodies with bad mesh references?
        // TODO(martinmo): (e.g. mesh has been destroyed but rigid body still alive)
        if (!sdb.isHandleValid(rigidBody->meshHandle))
        {
            continue;
        }

        auto rigidBodyPrivate = rigidBodiesPrivate.rel(rigidBodies, rigidBody);
        btRigidBody *bulletRigidBody = rigidBodyPrivate->state->bulletRigidBody.get();
        const btTransform &worldTrans = bulletRigidBody->getCenterOfMassTransform();

        rigidBody->linearVelocity  = fromBulletVec(bulletRigidBody->getLinearVelocity());
        rigidBody->angularVelocity = fromBulletVec(bulletRigidBody->getAngularVelocity());

        auto mesh = sdb.state< Renderer::Mesh::Info >(rigidBody->meshHandle);

        mesh->translation = fromBulletVec(worldTrans.getOrigin());
        mesh->rotation    = fromBulletQuat(worldTrans.getRotation());
    }

    // Check for destroyed rigid bodies
    trackDestructions(sdb, m_state->privateRigidBodies);
    // Check for destroyed constraints
    trackDestructions(sdb, m_state->privateConstraints);
}
