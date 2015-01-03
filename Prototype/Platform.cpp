// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "Platform.hpp"

#include "StateDb.hpp"
#include "Renderer.hpp"

// -------------------------------------------------------------------------------------------------
Platform::Platform(StateDb &stateDb) : stateDb(stateDb)
{
    // NOTE(MARTINMO): Think about separate 'Type' and 'State' structs that hold IDs
    RendererMesh = stateDb.registerType("RendererMesh");
    RendererMeshInfo = stateDb.registerState(
        RendererMesh, "RendererMeshInfo", sizeof(Renderer::MeshInfo));
}

// -------------------------------------------------------------------------------------------------
Platform::~Platform()
{
}
