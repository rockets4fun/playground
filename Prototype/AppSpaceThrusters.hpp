// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#ifndef APPSPACETHRUSTERS_HPP
#define APPSPACETHRUSTERS_HPP

#include "Common.hpp"
#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Space thrusters prototyping application module
struct AppSpaceThrusters : public ModuleIf
{
    AppSpaceThrusters();
    virtual ~AppSpaceThrusters();

public: // Application module interface implementation
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    u64 m_cameraHandle = 0;

private:
    COMMON_DISABLE_COPY(AppSpaceThrusters)
};

#endif
