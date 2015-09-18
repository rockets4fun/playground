// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef MODULE_IF_HPP
#define MODULE_IF_HPP

#include "Common.hpp"

struct StateDb;
struct Assets;
struct Renderer;

// -------------------------------------------------------------------------------------------------
/// @brief Application module interface
struct ModuleIf
{
    virtual void registerTypesAndStates(StateDb &sdb) = 0;
    virtual bool initialize(StateDb &sdb, Assets &assets) = 0;
    virtual void shutdown(StateDb &sdb) = 0;
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS) = 0;
};

#endif
