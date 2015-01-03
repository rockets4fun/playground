// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef MODULE_IF_HPP
#define MODULE_IF_HPP

#include "Common.hpp"

struct Platform;

// -------------------------------------------------------------------------------------------------
/// @brief Application module interface
struct ModuleIf
{
    virtual bool initialize(Platform &platform) = 0;
    virtual void shutdown() = 0;
    virtual void update(Platform &platform, real64 deltaTimeInS) = 0;
};

#endif
