// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "Common.hpp"

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Rigid body physics module
struct Physics : ModuleIf
{
    Physics();
    virtual ~Physics();

    u64 worldHandle() const;

    struct World
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            glm::fvec3 gravity;
        };
    };

    struct RigidBody
    {
        static u64 TYPE;
        enum CollisionShape
        {
            BOUNDING_BOX,
            BOUNDING_SPHERE,
            CONVEX_HULL_COMPOUND
        };
        enum Flag
        {
            RESET_TO_MESH = 0x1
        };
        struct Info
        {
            static u64 STATE;

            u32 flags = 0;

            u64 meshHandle = 0;
            u32 collisionShape = 0;

            u32 collisionGroup = 0;
            u32 collisionMask = 0;

            float mass = 0.0f;

            glm::fvec3 linearVelocity;
            glm::fvec3 linearVelocityLimit;

            glm::fvec3 angularVelocity;

            // TODO(martinmo): Add additional physics state here
            // TODO(martinmo): Translation and roation are already stored in mesh
        };
        struct PrivateInfo;
    };

    struct Constraint
    {
        static u64 TYPE;
        enum Type
        {
            FIXED
        };
        struct Info
        {
            static u64 STATE;

            u32 constraintType;

            u64 rigidBodyAHandle = 0;
            glm::fvec3 paramVecA;
            glm::fquat paramQuatA;

            u64 rigidBodyBHandle = 0;
            glm::fvec3 paramVecB;
            glm::fquat paramQuatB;
        };
        struct PrivateInfo;
    };

    struct Affector
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;

            u64 rigidBodyHandle = 0;
            u32 enabled = 0;

            // in affected RB's local coordinate frame
            glm::fvec3 forcePosition;

            // in global coordinate frame
            glm::fvec3 force;
            // in global coordinate frame
            glm::fvec3 torque;
        };
    };

    struct Sensor
    {
        static u64 TYPE;
        enum Type
        {
            ACCEL = 0, // Accelerometer - measures linear acceleration
            GYRO,      // Gyro - measures angular acceleration
            GPS        // GPS - measures XYZ position and XY speed
        };
        struct Info
        {
            static u64 STATE;
            // sensor type and value
            s32 type = 0;
            // ACCEL => Linear acceleration vector XYZ
            // GYRO  => Angular acceleration quaternion XYZW
            // GPS   => Position vector XYZ
            glm::fvec4 value;
            // GPS   => Horizontal velocity XY
            glm::fvec4 valueEx;
            // Sensor pose relative to rigid body it is attached to
            glm::fvec3 position;
            glm::fquat orientation;
            // Simulated sensor rigid body handle and error/noise
            u64        simRbHandle = 0;
            glm::fvec3 simBias;
            glm::fvec3 simGain;
            glm::fvec3 simNoise;
        };
    };

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    struct PrivateRigidBody;
    struct PrivateConstraint;
    struct PrivateState;

    std::shared_ptr< PrivateState > m_state;

    u64 m_worldHandle = 0;

    /*
#ifndef NDEBUG
    double m_nextPrintProfilingEventInS = 5.0;
#endif
    */

private:
    COMMON_DISABLE_COPY(Physics)
};

#endif
