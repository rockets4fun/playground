// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ROCKETSCIENCE_HPP
#define ROCKETSCIENCE_HPP

#include "Common.hpp"

#include <vector>
#include <map>

#include <glm/glm.hpp>

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
    static const int OCEAN_TILE_VERTEX_COUNT;
    static const float OCEAN_TILE_VERTEX_DIST;
    static const glm::fvec2 OCEAN_TILE_UNIT_SIZE;

    u64 m_cameraHandle = 0;
    u64 m_gridMeshHandle = 0;
    u64 m_arrowMeshHandle = 0;
    u64 m_pusherForceHandle = 0;

    u32 m_oceanModelAsset = 0;
    std::vector< u64 > m_oceanMeshHandles;

    std::vector< u64 > m_meshHandles;
    std::map< u64, u64 > m_rigidBodyByMeshHandle;

    std::vector< u64 > m_buoyancyForceHandles;

    double m_timeInS = 0.0;

    float oceanEquation(const glm::fvec2 &position, double timeInS);
    void updateBuoyancyForce(
        StateDb &stateDb, double timeInS, u64 buoyancyForceHandle);

private:
    COMMON_DISABLE_COPY(RocketScience);
};

#endif
