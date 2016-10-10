// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#include "AppSpaceThrusters.hpp"

#include "imgui.h"

#include "Math.hpp"
#include "Str.hpp"

#include "Profiler.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"

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
    auto spaceShipMesh = sdb.create< Renderer::Mesh::Info >(m_spaceShipMeshHandle);
    spaceShipMesh->translation = glm::fvec3(0.0f, 0.0f, 5.0f);
    spaceShipMesh->rotation = Math::rotateFromTo(
        glm::fvec3(0.0f, 1.0f, 0.0f), glm::fvec3(0.0f, 0.0f, 1.0f));
    spaceShipMesh->modelAsset = assets.asset("Assets/Models/PusherFront.model");
    spaceShipMesh->groups = Renderer::Group::DEFAULT;

    m_spaceShipMeshInit = *spaceShipMesh;

    auto spaceShipRb = sdb.create< Physics::RigidBody::Info >(m_spaceShipRbHandle);
    spaceShipRb->meshHandle = m_spaceShipMeshHandle;
    spaceShipRb->collisionShape = Physics::RigidBody::CollisionShape::CONVEX_HULL_COMPOUND;
    spaceShipRb->mass = 1.0;

    auto thrusterMesh = sdb.create< Renderer::Mesh::Info >(m_thrusterMeshHandle);
    thrusterMesh->modelAsset = assets.asset("Assets/Models/AxesY.model");
    thrusterMesh->groups = Renderer::Group::DEFAULT;

    auto thrusterAffector = sdb.create< Physics::Affector::Info >(m_thrusterAffectorHandle);
    thrusterAffector->rigidBodyHandle = m_spaceShipRbHandle;

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

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::imGuiUpdate(StateDb &sdb, Assets &assets)
{
    auto spaceShipMesh = sdb.state< Renderer::Mesh::Info >(m_spaceShipMeshHandle);
    auto spaceShipRb = sdb.state< Physics::RigidBody::Info >(m_spaceShipRbHandle);
    auto thrusterMesh = sdb.state< Renderer::Mesh::Info >(m_thrusterMeshHandle);
    auto thrusterAffector = sdb.state< Physics::Affector::Info >(m_thrusterAffectorHandle);

    ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Thruster");

    if (ImGui::Button("Reset Space Ship"))
    {
        *spaceShipMesh = m_spaceShipMeshInit;
        spaceShipRb->flags |= Physics::RigidBody::Flag::RESET_TO_MESH;
    }

    ImGui::Separator();

    thrusterMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
    thrusterAffector->enabled = false;

    Assets::Model *spaceShipModel = assets.refModel(spaceShipMesh->modelAsset);
    for (auto &instance : spaceShipModel->instances)
    {
        if (!Str::startsWith(instance.type, "Engine"))
        {
            continue;
        }

        glm::fvec3 translation;
        glm::fquat rotation;
        glm::fvec3 scale;
        Math::decomposeTransform(instance.xform, translation, rotation, scale);

        ImGui::Button(instance.name.c_str());

        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
        {
            thrusterAffector->enabled = true;
            thrusterAffector->forcePosition = translation;
            thrusterAffector->force = spaceShipMesh->rotation * glm::fvec3(-instance.xform[1]);
        }

        if (ImGui::IsItemHovered())
        {
            thrusterMesh->flags &= ~Renderer::Mesh::Flag::HIDDEN;
            thrusterMesh->translation =
                spaceShipMesh->translation + spaceShipMesh->rotation * translation;
            thrusterMesh->rotation = spaceShipMesh->rotation * rotation;
        }
    }

    ImGui::End();
}
