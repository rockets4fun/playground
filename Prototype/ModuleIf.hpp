// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef MODULE_IF_HPP
#define MODULE_IF_HPP

#include "Common.hpp"

struct StateDb;
struct Platform;

// -------------------------------------------------------------------------------------------------
/// @brief Application module interface
struct ModuleIf
{
    virtual void registerTypesAndStates(StateDb &stateDb) = 0;
    virtual bool initialize(Platform &platform) = 0;
    virtual void shutdown(Platform &platform) = 0;
    virtual void update(Platform &platform, double deltaTimeInS) = 0;
};

#endif
