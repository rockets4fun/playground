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

    struct RigidBody
    {
        static u64 TYPE;
        struct Info
        {
            enum CollisionShapeType
            {
                BOUNDING_BOX,
                BOUNDING_SPHERE,
                CONVEX_HULL_COMPOUND
            };

            static u64 STATE;
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

private:
    COMMON_DISABLE_COPY(Physics);
};

#endif
