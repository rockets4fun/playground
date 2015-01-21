// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ROCKETSCIENCE_HPP
#define ROCKETSCIENCE_HPP

#include "Common.hpp"

#include <vector>
#include <map>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Rocket science prototype logic module
struct RocketScience : public ModuleIf
{
    RocketScience();
    virtual ~RocketScience();

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &stateDb);
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, double deltaTimeInS);

private:
    u64 m_cameraHandle = 0;
    u64 m_gridMeshHandle = 0;
    u64 m_arrowMeshHandle = 0;
    u64 m_pusherForce = 0;

    std::vector< u64 > m_meshHandles;
    std::map< u64, u64 > m_rigidBodyByMeshHandle;

private:
    COMMON_DISABLE_COPY(RocketScience);
};

#endif
