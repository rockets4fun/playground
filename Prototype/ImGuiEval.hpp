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

public: // Application module interface
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    std::vector< u64 > m_meshHandles;
    u32 m_fontTextureAsset = 0;

    bool m_testWindowVisible = true;

private:
    COMMON_DISABLE_COPY(ImGuiEval)
};

#endif
