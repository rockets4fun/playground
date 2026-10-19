// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#ifndef APPSPACETHRUSTERS_HPP
#define APPSPACETHRUSTERS_HPP

#include <string>

#include "Common.hpp"
#include "ModuleIf.hpp"
#include "ImGuiEval.hpp"
#include "Renderer.hpp"

struct Physics;

// -------------------------------------------------------------------------------------------------
/// @brief Space thrusters prototyping application module
struct AppSpaceThrusters : public ModuleIf, public ImGuiEval::ModuleIf
{
    AppSpaceThrusters( Physics& physics, ImGuiEval& imGui );
    virtual ~AppSpaceThrusters();

public:  // Application module interface implementation
    virtual void registerTypesAndStates( StateDb& sdb );
    virtual bool initialize( StateDb& sdb, Assets& assets );
    virtual void shutdown( StateDb& sdb );
    virtual void update( StateDb& sdb, Assets& assets, Renderer& renderer, double deltaTimeInS );

public:  // ImGui module interface implementation
    virtual void imGuiUpdate( StateDb& sdb, Assets& assets );

private:
    struct Thruster
    {
        std::string name;
        u64 infoMeshHandle = 0;
        u64 affectorHandle = 0;
        // Thruster position and direction in space ship local frame
        glm::fvec3 pos;
        glm::fvec3 dir;
    };

    struct ThrusterCombo
    {
        // Bit i set => thruster i fires
        u32 mask  = 0;
        u32 count = 0;
        // Combined torque (about ship origin) and force in space ship local frame
        glm::fvec3 torque;
        glm::fvec3 force;
    };

    struct AutoPilot
    {
        // Target orientation as intrinsic yaw (Z), pitch (X), roll (Y) applied to initial orientation
        glm::fvec3 targetYprInDeg = glm::fvec3( 0.0f );
        // Fraction of max angular acceleration assumed to be available for braking
        float brakingFactor = 0.5f;
        float angleDeadbandInDeg    = 0.5f;
        float rateDeadbandInDegPerS = 1.0f;
        float maxRateInDegPerS      = 30.0f;
        // Penalty weight for net force of thruster combo (0 => ship may drift while re-orienting)
        float forceWeight = 0.0f;
        // Debug output (space ship local frame)
        glm::fvec3 errorInDeg          = glm::fvec3( 0.0f );
        glm::fvec3 rateInDegPerS       = glm::fvec3( 0.0f );
        glm::fvec3 targetRateInDegPerS = glm::fvec3( 0.0f );
        glm::fvec3 accelMax            = glm::fvec3( 0.0f );
        u32 mask                       = 0;
    };

    u32 autoPilotUpdate( const glm::fquat& rotation, const glm::fvec3& angularVelocity, const glm::fvec3& inertia );

    Physics* m_physics = nullptr;
    ImGuiEval* m_imGui = nullptr;

    u64 m_cameraHandle = 0;

    u64 m_spaceShipMeshHandle = 0;
    Renderer::Mesh::Info m_spaceShipMeshInit;
    u64 m_spaceShipRbHandle = 0;

    std::vector< Thruster > m_thrusters;
    std::vector< ThrusterCombo > m_thrusterCombos;

    AutoPilot m_autoPilot;
    double m_deltaTimeInS = 1.0 / 60.0;

    u64 m_testAffector = 0;

private:
    COMMON_DISABLE_COPY( AppSpaceThrusters )
};

#endif
