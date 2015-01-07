// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 07.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "Common.hpp"

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief ...
struct Physics : ModuleIf
{
    Physics();
    virtual ~Physics();

public: // Implementation of module interface
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, real64 deltaTimeInS);

private:
    COMMON_DISABLE_COPY(Physics);
};

#endif
