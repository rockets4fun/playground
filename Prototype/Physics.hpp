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
            static u64 STATE;
            u64 rendererMeshId = 0;
            // TODO(MARTINMO): Add additional physics state here
            // TODO(MARTINMO): Orientation and position are already stored in mesh
        };
        struct InternalInfo;
    };

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &stateDb);
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, double deltaTimeInS);

private:
    struct State;

    std::shared_ptr< State > state;

private:
    COMMON_DISABLE_COPY(Physics);
};

#endif
