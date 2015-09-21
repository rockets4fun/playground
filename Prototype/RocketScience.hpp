// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ROCKETSCIENCE_HPP
#define ROCKETSCIENCE_HPP

#include "Common.hpp"

#include <list>

#include <glm/glm.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Rocket science prototype logic module
struct RocketScience : public ModuleIf
{
    RocketScience();
    virtual ~RocketScience();

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    static const int OCEAN_TILE_VERTEX_COUNT;
    static const float OCEAN_TILE_VERTEX_DIST;
    static const glm::fvec2 OCEAN_TILE_UNIT_SIZE;

    static const int PLATFORM_SPHERE_COUNT;

    u64 m_cameraHandle = 0;

    u32 m_oceanModelAsset = 0;
    u32 m_uiModelAsset = 0;
    u32 m_postModelAsset = 0;

    u64 m_arrowMeshHandle = 0;
    u64 m_pusherAffectorHandle = 0;

    std::list< u64 > m_sleepingMeshHandles;
    std::list< u64 > m_buoyancyAffectorHandles;

    double m_timeInS = 0.0;

    float oceanEquation(const glm::fvec2 &position, double timeInS);

    void addBuoyancyAffector(StateDb &sdb,
        const glm::fvec3 &translation, u64 parentRigidBody);
    void updateBuoyancyAffectors(StateDb &sdb, double timeInS);

private:
    COMMON_DISABLE_COPY(RocketScience)
};

#endif
