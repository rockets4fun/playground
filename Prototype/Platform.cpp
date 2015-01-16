// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "Platform.hpp"

#include "StateDb.hpp"
#include "Renderer.hpp"

// -------------------------------------------------------------------------------------------------
Platform::Platform(StateDb &stateDb, Assets &assets, Renderer &renderer) :
    stateDb(stateDb), assets(assets), renderer(renderer)
{
}

// -------------------------------------------------------------------------------------------------
Platform::~Platform()
{
}
