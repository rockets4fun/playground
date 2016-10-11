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

    Assets::Model *spaceShipModel = assets.refModel(spaceShipMesh->modelAsset);
    for (auto &instance : spaceShipModel->instances)
    {
        if (!Str::startsWith(instance.type, "Engine"))
        {
            continue;
        }

        Thruster thruster;

        thruster.name = instance.name;

        auto infoMesh = sdb.create< Renderer::Mesh::Info >(thruster.infoMeshHandle);
        infoMesh->modelAsset = assets.asset("Assets/Models/AxesY.model");
        infoMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
        infoMesh->groups = Renderer::Group::DEFAULT;

        auto affector = sdb.create< Physics::Affector::Info >(thruster.affectorHandle);
        affector->rigidBodyHandle = m_spaceShipRbHandle;

        glm::fvec3 translation;
        glm::fquat rotation;
        glm::fvec3 scale;
        Math::decomposeTransform(instance.xform, translation, rotation, scale);

        thruster.pos = translation;
        thruster.dir = -instance.xform[1];

        m_thrusters.push_back(thruster);
    }

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

    ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Thrusters");

    if (ImGui::Button("Reset Space Ship"))
    {
        *spaceShipMesh = m_spaceShipMeshInit;
        spaceShipRb->flags |= Physics::RigidBody::Flag::RESET_TO_MESH;
    }

    /*
    std::set< std::string > activeThrusters;
    ImGui::Separator();
    if (ImGui::Button("Roll Left"))
    {
        activeThrusters.insert("Front-Wing-Left.Engine-Top");
        activeThrusters.insert("Front-Wing-Right.Engine-Bottom");
    }
    */

    ImGui::Separator();
    for (auto &thruster : m_thrusters)
    {
        ImGui::Button(thruster.name.c_str());

        auto infoMesh = sdb.state< Renderer::Mesh::Info >(thruster.infoMeshHandle);
        auto affector = sdb.state< Physics::Affector::Info >(thruster.affectorHandle);

        // Default is for info mesh to be hidden and affector to be disabled
        infoMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
        affector->enabled = false;

        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
        {
            affector->enabled = true;
            // FIXME(MARTINMO): Should also be global just like 'force'
            affector->forcePosition = thruster.pos;
            affector->force = spaceShipMesh->rotation * thruster.dir;
        }

        if (ImGui::IsItemHovered())
        {
            infoMesh->flags &= ~Renderer::Mesh::Flag::HIDDEN;
            infoMesh->translation =
                spaceShipMesh->translation + spaceShipMesh->rotation * thruster.pos;
            infoMesh->rotation = spaceShipMesh->rotation
                * Math::rotateFromTo(glm::fvec3(0.0f, 1.0f, 0.0f), thruster.dir);
        }
    }

    ImGui::End();
}