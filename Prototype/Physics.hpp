// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "Common.hpp"

#include <memory>

#include <glm/glm.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief ...
struct Physics : ModuleIf
{
    Physics();
    virtual ~Physics();

    struct Force
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            u64 rigidBodyHandle;
            u32 enabled;
            glm::fvec3 force;
            glm::fvec3 position;
        };
    };

    struct RigidBody
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            enum CollisionShapeType
            {
                BOUNDING_BOX,
                BOUNDING_SPHERE,
                CONVEX_HULL_COMPOUND
            };
            u64 meshHandle = 0;
            u32 collisionShapeType = 0;
            // TODO(martinmo): Add additional physics state here
            // TODO(martinmo): Translation and roation are already stored in mesh
        };
        struct PrivateInfo;
    };

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &stateDb);
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, double deltaTimeInS);

private:
    struct PrivateRigidBody;
    struct PrivateState;

    std::shared_ptr< PrivateState > state;

    /*
#ifndef NDEBUG
    double m_nextPrintProfilingEventInS = 5.0;
#endif
    */

private:
    COMMON_DISABLE_COPY(Physics);
};

#endif
