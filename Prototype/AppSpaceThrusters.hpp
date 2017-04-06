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
struct AppSpaceThrusters :
    public ModuleIf,
    public ImGuiEval::ModuleIf
{
    AppSpaceThrusters(Physics &physics, ImGuiEval &imGui);
    virtual ~AppSpaceThrusters();

public: // Application module interface implementation
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

public: // ImGui module interface implementation
    virtual void imGuiUpdate(StateDb &sdb, Assets &assets);

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

    Physics *m_physics = nullptr;
    ImGuiEval *m_imGui = nullptr;

    u64 m_cameraHandle = 0;

    u64 m_spaceShipMeshHandle = 0;
    Renderer::Mesh::Info m_spaceShipMeshInit;
    u64 m_spaceShipRbHandle = 0;

    std::vector< Thruster > m_thrusters;

    u64 m_testAffector;

private:
    COMMON_DISABLE_COPY(AppSpaceThrusters)
};

#endif
