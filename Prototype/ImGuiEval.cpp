// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 22.12.2015
// -------------------------------------------------------------------------------------------------

#include "ImGuiEval.hpp"

// -------------------------------------------------------------------------------------------------
ImGuiEval::ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
ImGuiEval::~ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::registerTypesAndStates(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
bool ImGuiEval::initialize(StateDb &sdb, Assets &assets)
{
    return true;
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::shutdown(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    // (1) Define the user interface
    {
    }

    // (2) Render UI into command buffers
    {
    }

    // (3) Retrieve command buffers and transform for renderer
    {
    }
}
