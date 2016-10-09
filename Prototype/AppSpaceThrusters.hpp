// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#ifndef APPSPACETHRUSTERS_HPP
#define APPSPACETHRUSTERS_HPP

#include "Common.hpp"
#include "ModuleIf.hpp"
#include "ImGuiEval.hpp"
#include "Renderer.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Space thrusters prototyping application module
struct AppSpaceThrusters :
    public ModuleIf,
    public ImGuiEval::ModuleIf
{
    AppSpaceThrusters();
    virtual ~AppSpaceThrusters();

public: // Application module interface implementation
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

public: // ImGui module interface implementation
    virtual void imGuiUpdate(StateDb &sdb, Assets &assets);

private:
    u64 m_cameraHandle = 0;

    u64 m_spaceShipMeshHandle = 0;
    Renderer::Mesh::Info m_spaceShipMeshInit;

    u64 m_spaceShipRbHandle = 0;
    u64 m_thrusterMeshHandle = 0;
    u64 m_thrusterAffectorHandle = 0;

private:
    COMMON_DISABLE_COPY(AppSpaceThrusters)
};

#endif
