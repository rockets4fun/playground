// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 22.12.2015
// -------------------------------------------------------------------------------------------------

#ifndef IMGUIEVAL_HPP
#define IMGUIEVAL_HPP

#include "Common.hpp"
#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Brief implementation module description
struct ImGuiEval : public ModuleIf
{
    ImGuiEval();
    virtual ~ImGuiEval();

public: // Application module interface
    virtual void registerTypesAndStates(StateDb &sdb) = 0;
    virtual bool initialize(StateDb &sdb, Assets &assets) = 0;
    virtual void shutdown(StateDb &sdb) = 0;
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS) = 0;

private:
    COMMON_DISABLE_COPY(ImGuiEval)
};

#endif
