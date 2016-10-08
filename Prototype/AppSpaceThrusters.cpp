// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#include "AppSpaceThrusters.hpp"

#include "Math.hpp"
#include "Profiler.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"

// -------------------------------------------------------------------------------------------------
AppSpaceThrusters::AppSpaceThrusters()
{
}

// -------------------------------------------------------------------------------------------------
AppSpaceThrusters::~AppSpaceThrusters()
{
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::registerTypesAndStates(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
bool AppSpaceThrusters::initialize(StateDb &sdb, Assets &assets)
{
    auto spaceShipMesh = sdb.create< Renderer::Mesh::Info >();
    spaceShipMesh->translation = glm::fvec3(0.0f, 0.0f, 5.0f);
    spaceShipMesh->rotation = Math::rotateFromTo(
        glm::fvec3(0.0f, 1.0f, 0.0f), glm::fvec3(0.0f, 0.0f, 1.0f));
    spaceShipMesh->modelAsset = assets.asset("Assets/Models/PusherFront.model");
    spaceShipMesh->groups = Renderer::Group::DEFAULT;

    auto mesh = sdb.create< Renderer::Mesh::Info >();
    mesh->modelAsset = assets.asset("Assets/Models/AxesXYZ.model");
    mesh->groups = Renderer::Group::DEFAULT;

    auto camera = sdb.create< Renderer::Camera::Info >(m_cameraHandle);
    camera->position = glm::fvec3(0.0f, -10.0f, 10.0f);
    camera->target   = glm::fvec3(0.0f,   0.0f,  5.0f);

    return true;
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::shutdown(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::update(
    StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    PROFILER_SECTION(AppSpaceThrusters, glm::fvec3(0.0f, 1.0f, 0.0f))

    renderer.activeCameraHandle = m_cameraHandle;
}
