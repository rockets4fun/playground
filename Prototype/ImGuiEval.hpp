// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 22.12.2015
// -------------------------------------------------------------------------------------------------

#ifndef IMGUIEVAL_HPP
#define IMGUIEVAL_HPP

#include "Common.hpp"

#include <vector>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Brief implementation module description
struct ImGuiEval : public ModuleIf
{
    ImGuiEval();
    virtual ~ImGuiEval();

    struct ModuleIf
    {
    public: // ImGui module interface
        virtual void imGuiUpdate(StateDb &sdb, Assets &assets) = 0;
    };

    void registerModule(ModuleIf &module);

public: // Application module interface
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    std::vector< u64 > m_meshHandles;
    u64 m_fontTextureHandle = 0;
    u64 m_programHandle = 0;
    u64 m_passHandle = 0;

    bool m_metricsVisible = true;
    bool m_profilerVisible = true;

    std::vector< ModuleIf * > m_modules;

private:
    COMMON_DISABLE_COPY(ImGuiEval)
};

#endif
