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
    // TODO(MARTINMO): Think about separate 'Type' and 'State' structs that hold IDs

    RendererMesh = stateDb.registerType("RendererMesh", 4096);
    RendererMeshInfo = stateDb.registerState(
        RendererMesh, "RendererMeshInfo", sizeof(Renderer::MeshInfo));

    RendererCamera = stateDb.registerType("RendererCamera");
    RendererCameraInfo = stateDb.registerState(
        RendererCamera, "RendererCameraInfo", sizeof(Renderer::CameraInfo));
}

// -------------------------------------------------------------------------------------------------
Platform::~Platform()
{
}
