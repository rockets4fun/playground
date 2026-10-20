// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 08.10.2016
// -------------------------------------------------------------------------------------------------

#include "AppSpaceThrusters.hpp"

#include <bitset>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <limits>

#include "imgui.h"

#include "Math.hpp"
#include "Str.hpp"

#include "Profiler.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"

// -------------------------------------------------------------------------------------------------
AppSpaceThrusters::AppSpaceThrusters( Physics& physics, ImGuiEval& imGui )
    : m_physics( &physics )
{
    imGui.registerModule( *this );
}

// -------------------------------------------------------------------------------------------------
AppSpaceThrusters::~AppSpaceThrusters()
{
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::registerTypesAndStates( StateDb& sdb )
{
}

// -------------------------------------------------------------------------------------------------
bool AppSpaceThrusters::initialize( StateDb& sdb, Assets& assets )
{
    auto spaceShipMesh         = sdb.create< Renderer::Mesh::Info >( m_spaceShipMeshHandle );
    spaceShipMesh->translation = glm::fvec3( 0.0f, 0.0f, 5.0f );
    spaceShipMesh->rotation =
        Math::rotateFromTo( glm::fvec3( 0.0f, 1.0f, 0.0f ), glm::fvec3( 0.0f, 0.0f, 1.0f ) );
    spaceShipMesh->modelAsset = assets.asset( "Assets/Models/PusherFront.model" );
    spaceShipMesh->groups     = Renderer::Group::DEFAULT;

    m_spaceShipMeshInit = *spaceShipMesh;

    auto spaceShipRb            = sdb.create< Physics::RigidBody::Info >( m_spaceShipRbHandle );
    spaceShipRb->meshHandle     = m_spaceShipMeshHandle;
    spaceShipRb->collisionShape = Physics::RigidBody::CollisionShape::CONVEX_HULL_COMPOUND;
    spaceShipRb->mass           = 1.0;

    Assets::Model* spaceShipModel = assets.refModel( spaceShipMesh->modelAsset );
    for ( auto& instance : spaceShipModel->instances ) {
        if ( !Str::startsWith( instance.type, "Engine" ) ) {
            continue;
        }

        Thruster thruster;

        thruster.name = instance.name;

        auto infoMesh        = sdb.create< Renderer::Mesh::Info >( thruster.infoMeshHandle );
        infoMesh->modelAsset = assets.asset( "Assets/Models/AxesY.model" );
        infoMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
        infoMesh->groups = Renderer::Group::DEFAULT;

        auto affector             = sdb.create< Physics::Affector::Info >( thruster.affectorHandle );
        affector->rigidBodyHandle = m_spaceShipRbHandle;

        glm::fvec3 translation;
        glm::fquat rotation;
        glm::fvec3 scale;
        Math::decomposeTransform( instance.xform, translation, rotation, scale );

        thruster.pos = translation;
        thruster.dir = glm::fvec3( -instance.xform[ 1 ] );

        m_thrusters.push_back( thruster );
    }

    // Pre-compute all thruster combinations (including the empty one) for the auto pilot
    COMMON_ASSERT( m_thrusters.size() <= 16 );
    for ( u32 mask = 0; mask < ( 1u << m_thrusters.size() ); ++mask ) {
        ThrusterCombo combo;
        combo.mask   = mask;
        combo.torque = glm::fvec3( 0.0f );
        combo.force  = glm::fvec3( 0.0f );
        for ( size_t idx = 0; idx < m_thrusters.size(); ++idx ) {
            if ( !( mask & ( 1u << idx ) ) ) {
                continue;
            }
            const Thruster& thruster = m_thrusters[ idx ];
            combo.torque += glm::cross( thruster.pos, thruster.dir );
            combo.force += thruster.dir;
            ++combo.count;
        }
        m_thrusterCombos.push_back( combo );
    }

    auto affector             = sdb.create< Physics::Affector::Info >( m_testAffector );
    affector->rigidBodyHandle = m_spaceShipRbHandle;

    auto mesh        = sdb.create< Renderer::Mesh::Info >();
    mesh->modelAsset = assets.asset( "Assets/Models/AxesXYZ.model" );
    mesh->groups     = Renderer::Group::DEFAULT;

    auto camera      = sdb.create< Renderer::Camera::Info >( m_cameraHandle );
    camera->position = glm::fvec3( 0.0f, -10.0f, 10.0f );
    camera->target   = glm::fvec3( 0.0f, 0.0f, 5.0f );

    auto world     = sdb.state< Physics::World::Info >( m_physics->worldHandle() );
    world->gravity = glm::fvec3( 0.0f, 0.0f, 0.0f );

    auto sensor         = sdb.create< Physics::Sensor::Info >();
    sensor->type        = Physics::Sensor::ACCEL;
    sensor->simRbHandle = m_spaceShipRbHandle;

    return true;
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::shutdown( StateDb& sdb )
{
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::update( StateDb& sdb, Assets& assets, Renderer& renderer, double deltaTimeInS )
{
    PROFILER_SECTION( AppSpaceThrusters, glm::fvec3( 0.0f, 1.0f, 0.0f ) )

    renderer.activeCameraHandle = m_cameraHandle;

    m_deltaTimeInS = deltaTimeInS;
}

// -------------------------------------------------------------------------------------------------
/// @brief Selects thruster combo to fire for one frame to rotate towards and hold target orientation
/// @param rotation Space ship rotation (local => global)
/// @param angularVelocity Space ship angular velocity in global frame
/// @param linearVelocity Space ship linear velocity in global frame
/// @param inertia Space ship diagonal inertia tensor in local frame
/// @param mass Space ship mass
/// @return Thruster mask (bit i set => thruster i fires)
///
/// Phase-plane switching rule about the error axis (eigenaxis): a target rate is derived from the
/// remaining angle such that the ship can still brake in time. The required angular acceleration to
/// reach that rate within the frame is then matched as closely as possible by brute-forcing over all
/// thruster combos. Combos whose net force also counters the ship's velocity are preferred.
u32 AppSpaceThrusters::autoPilotUpdate(
    const glm::fquat& rotation, const glm::fvec3& angularVelocity, const glm::fvec3& linearVelocity,
    const glm::fvec3& inertia, float mass )
{
    AutoPilot& ap = m_autoPilot;
    ap.mask       = 0;

    if ( inertia.x <= 0.0f || inertia.y <= 0.0f || inertia.z <= 0.0f || mass <= 0.0f ) {
        return 0;
    }

    const float deltaTimeInS = float( m_deltaTimeInS );

    // Target rotation (local => global)
    const glm::fvec3 ypr     = glm::radians( ap.targetYprInDeg );
    const glm::fquat target = m_spaceShipMeshInit.rotation * glm::angleAxis( ypr.x, Math::Z_AXIS )
                              * glm::angleAxis( ypr.y, Math::X_AXIS ) * glm::angleAxis( ypr.z, Math::Y_AXIS );

    // Rotation error as rotation vector (axis * angle) in local frame along shortest path
    glm::fquat errorQuat = glm::conjugate( rotation ) * target;
    if ( errorQuat.w < 0.0f ) {
        errorQuat = -errorQuat;
    }
    const glm::fvec3 errorAxis( errorQuat.x, errorQuat.y, errorQuat.z );
    const float sinHalfAngle = glm::length( errorAxis );
    glm::fvec3 error( 0.0f );
    if ( sinHalfAngle > 1e-6f ) {
        error = errorAxis * ( 2.0f * std::atan2( sinHalfAngle, errorQuat.w ) / sinHalfAngle );
    }

    const glm::fvec3 rate = glm::conjugate( rotation ) * angularVelocity;

    // Per-axis angular and linear acceleration authority (weaker direction of both)
    glm::fvec3 accelMaxPos( 0.0f );
    glm::fvec3 accelMaxNeg( 0.0f );
    glm::fvec3 linearAccelMaxPos( 0.0f );
    glm::fvec3 linearAccelMaxNeg( 0.0f );
    for ( const ThrusterCombo& combo : m_thrusterCombos ) {
        const glm::fvec3 accel       = combo.torque / inertia;
        const glm::fvec3 linearAccel = combo.force / mass;
        accelMaxPos                  = glm::max( accelMaxPos, accel );
        accelMaxNeg                  = glm::max( accelMaxNeg, -accel );
        linearAccelMaxPos            = glm::max( linearAccelMaxPos, linearAccel );
        linearAccelMaxNeg            = glm::max( linearAccelMaxNeg, -linearAccel );
    }
    const glm::fvec3 accelMax = glm::max( glm::min( accelMaxPos, accelMaxNeg ), glm::fvec3( 1e-6f ) );
    const glm::fvec3 linearAccelMax =
        glm::max( glm::min( linearAccelMaxPos, linearAccelMaxNeg ), glm::fvec3( 1e-6f ) );

    // Switching rule => desired angular acceleration
    const float angleDeadband = glm::radians( ap.angleDeadbandInDeg );
    const float rateDeadband  = glm::radians( ap.rateDeadbandInDegPerS );
    const float maxRate       = glm::radians( ap.maxRateInDegPerS );
    const float errorAngle    = glm::length( error );
    const float angle         = glm::max( errorAngle - angleDeadband, 0.0f );
    glm::fvec3 targetRate( 0.0f );
    glm::fvec3 accelDemand( 0.0f );
    if ( angle > 0.0f ) {
        // Rotate about error axis (eigenaxis) so all axes accelerate and brake in sync which keeps the
        // selected thruster combo stable, while damping any rate across the error axis
        const glm::fvec3 errorDir   = error / errorAngle;
        const float rateAlong       = glm::dot( rate, errorDir );
        const glm::fvec3 rateAcross = rate - rateAlong * errorDir;

        // Angular acceleration authority along error axis limited by weakest involved local axis
        float accelAlong = std::numeric_limits< float >::max();
        for ( int axis = 0; axis < 3; ++axis ) {
            if ( glm::abs( errorDir[ axis ] ) > 1e-6f ) {
                accelAlong = glm::min( accelAlong, accelMax[ axis ] / glm::abs( errorDir[ axis ] ) );
            }
        }

        const float targetRateAlong = glm::min( glm::sqrt( 2.0f * ap.brakingFactor * accelAlong * angle ), maxRate );
        targetRate                  = targetRateAlong * errorDir;

        accelDemand = glm::clamp( ( targetRateAlong - rateAlong ) / deltaTimeInS, -accelAlong, accelAlong ) * errorDir
                      + glm::clamp( -rateAcross / deltaTimeInS, -accelMax, accelMax );
        accelDemand = glm::clamp( accelDemand, -accelMax, accelMax );
    }
    else {
        // Holding: rate deadband avoids thrusters chattering (only applied while holding to keep
        // thrusters firing continuously while braking along the switching curve)
        for ( int axis = 0; axis < 3; ++axis ) {
            if ( glm::abs( rate[ axis ] ) >= rateDeadband ) {
                accelDemand[ axis ] =
                    glm::clamp( -rate[ axis ] / deltaTimeInS, -accelMax[ axis ], accelMax[ axis ] );
            }
        }
    }

    // Velocity hold => desired linear acceleration in local frame to null velocity within the frame
    // (gentler demands would always be matched best by not firing at all as even the weakest combo
    // exceeds them, so the velocity would never be nulled)
    const glm::fvec3 localVelocity = glm::conjugate( rotation ) * linearVelocity;
    glm::fvec3 linearAccelDemand   = glm::clamp( -localVelocity / deltaTimeInS, -linearAccelMax, linearAccelMax );
    for ( int axis = 0; axis < 3; ++axis ) {
        if ( glm::abs( localVelocity[ axis ] ) < ap.velocityDeadband ) {
            linearAccelDemand[ axis ] = 0.0f;
        }
    }
    // Local X force only comes from thrusters also causing roll torque so countering local X velocity
    // would fight the rotation (net local X force is still penalized by demanding zero)
    linearAccelDemand.x = 0.0f;

    // Pick thruster combo closest to desired angular acceleration (fewer thrusters on ties) while
    // penalizing thrusters switched ON compared to previous frame against ON/OFF chattering (switching
    // OFF is not penalized as it would keep thrusters firing needlessly, causing drift)
    float bestCost = std::numeric_limits< float >::max();
    for ( const ThrusterCombo& combo : m_thrusterCombos ) {
        const glm::fvec3 delta       = ( combo.torque / inertia - accelDemand ) / accelMax;
        const glm::fvec3 linearDelta = ( combo.force / mass - linearAccelDemand ) / linearAccelMax;
        const size_t switchedOn      = std::bitset< 32 >( combo.mask & ~ap.firedMask ).count();
        const float cost = glm::dot( delta, delta ) + ap.forceWeight * glm::dot( combo.force, combo.force )
                           + ap.velocityWeight * glm::dot( linearDelta, linearDelta )
                           + ap.switchWeight * float( switchedOn ) + 1e-3f * float( combo.count );
        if ( cost < bestCost ) {
            bestCost = cost;
            ap.mask  = combo.mask;
        }
    }

    ap.errorInDeg          = glm::degrees( error );
    ap.rateInDegPerS       = glm::degrees( rate );
    ap.targetRateInDegPerS = glm::degrees( targetRate );
    ap.accelMax            = accelMax;
    ap.speed               = glm::length( linearVelocity );

    return ap.mask;
}

// -------------------------------------------------------------------------------------------------
void AppSpaceThrusters::imGuiUpdate( StateDb& sdb, Assets& assets )
{
    auto spaceShipMesh = sdb.state< Renderer::Mesh::Info >( m_spaceShipMeshHandle );
    auto spaceShipRb   = sdb.state< Physics::RigidBody::Info >( m_spaceShipRbHandle );

    bool mouseDown = ImGui::IsMouseDown( 0 );

    ImGui::SetNextWindowSize( ImVec2( 200, 100 ), ImGuiCond_FirstUseEver );
    ImGui::Begin( "Thrusters" );

    if ( ImGui::Button( "Reset Space Ship" ) ) {
        *spaceShipMesh = m_spaceShipMeshInit;
        spaceShipRb->flags |= Physics::RigidBody::Flag::RESET_TO_MESH;
    }

    std::set< std::string > activeThrusters;

    ImGui::Separator();
    {
        ImGui::Button( "Roll Left" );
        if ( ImGui::IsItemHovered() && mouseDown ) {
            activeThrusters.insert( "Front-Wing-Left.Engine-Top" );
            activeThrusters.insert( "Front-Wing-Right.Engine-Bottom" );
        }
    }
    {
        ImGui::Button( "Roll Right" );
        if ( ImGui::IsItemHovered() && mouseDown ) {
            activeThrusters.insert( "Front-Wing-Left.Engine-Bottom" );
            activeThrusters.insert( "Front-Wing-Right.Engine-Top" );
        }
    }
    ImGui::Separator();
    {
        AutoPilot& ap = m_autoPilot;

        // Fixed-width label column, remaining elements of each line fill up the rest of the window
        const ImGuiStyle& style = ImGui::GetStyle();
        const float labelWidth  = ImGui::CalcTextSize( "Switch Weight" ).x + 2.0f * style.ItemSpacing.x;
        auto label              = [ & ]( const char* text, const char* tooltip ) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted( text );
            if ( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s", tooltip );
            }
            ImGui::SameLine( labelWidth );
        };
        // Space right of the target yaw, pitch, roll fields taken up by the "Auto" button
        const float autoWidth = ImGui::CalcTextSize( "Auto" ).x + 2.0f * style.FramePadding.x + style.ItemSpacing.x;
        // Values are centered in the same three cells used by 'DragFloat3' (yaw, pitch, roll)
        auto values = [ & ]( const char* text, const char* tooltip, const glm::fvec3& ypr, const char* format ) {
            label( text, tooltip );
            const float startX    = ImGui::GetCursorPosX();
            const float cellWidth = std::floor(
                ( ImGui::GetContentRegionAvail().x - autoWidth - 2.0f * style.ItemInnerSpacing.x ) / 3.0f );
            for ( int idx = 0; idx < 3; ++idx ) {
                char buffer[ 32 ];
                snprintf( buffer, sizeof( buffer ), format, ypr[ idx ] );
                const float textWidth = ImGui::CalcTextSize( buffer ).x;
                ImGui::SameLine(
                    startX + float( idx ) * ( cellWidth + style.ItemInnerSpacing.x )
                    + 0.5f * ( cellWidth - textWidth ) );
                ImGui::TextUnformatted( buffer );
            }
        };

        label( "Target", "Target yaw, pitch, roll (deg)" );
        ImGui::SetNextItemWidth( -autoWidth );
        ImGui::DragFloat3( "##Target", &ap.targetYprInDeg.x, 1.0f, -180.0f, 180.0f, "%.0f" );

        // Thrusters only fire according to auto pilot while button is pressed
        ImGui::SameLine();
        ImGui::Button( "Auto" );
        bool autoActive = ImGui::IsItemHovered() && mouseDown;

        u32 mask = autoPilotUpdate(
            spaceShipMesh->rotation, spaceShipRb->angularVelocity, spaceShipRb->linearVelocity, spaceShipRb->inertia,
            spaceShipRb->mass );
        u32 firedMask = autoActive ? mask : 0;
        for ( size_t idx = 0; idx < m_thrusters.size(); ++idx ) {
            if ( firedMask & ( 1u << idx ) ) {
                activeThrusters.insert( m_thrusters[ idx ].name );
            }
        }

        // Count thrusters switched ON by auto pilot over one second windows
        ap.onCycleCount += u32( std::bitset< 32 >( firedMask & ~ap.firedMask ).count() );
        ap.onCycleTimeInS += float( m_deltaTimeInS );
        ap.firedMask = firedMask;
        if ( ap.onCycleTimeInS >= 1.0f ) {
            ap.onCyclesPerS   = float( ap.onCycleCount ) / ap.onCycleTimeInS;
            ap.onCycleCount   = 0;
            ap.onCycleTimeInS = 0.0f;
        }

        // Local X, Y, Z => yaw (Z), pitch (X), roll (Y)
        auto toYpr = []( const glm::fvec3& v ) { return glm::fvec3( v.z, v.x, v.y ); };

        values( "Error", "Orientation error (deg)", toYpr( ap.errorInDeg ), "%.1f" );
        values( "Rate", "Angular rate (deg/s)", toYpr( ap.rateInDegPerS ), "%.1f" );
        values( "Target Rate", "Target angular rate (deg/s)", toYpr( ap.targetRateInDegPerS ), "%.1f" );

        label( "ON-Cycles", "Thrusters switched ON by auto pilot per second (1 s window)" );
        ImGui::Text( "%.0f /s", ap.onCyclesPerS );

        label( "Speed", "Space ship linear speed (m/s)" );
        ImGui::Text( "%.2f m/s", ap.speed );

        // No tree push to keep label column aligned with the lines above
        if ( ImGui::TreeNodeEx( "Auto Tuning", ImGuiTreeNodeFlags_NoTreePushOnOpen ) ) {
            label( "Braking", "Fraction of max angular acceleration assumed to be available for braking" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##Braking", &ap.brakingFactor, 0.1f, 1.0f, "%.2f" );

            label( "Angle Band", "Angle deadband around target orientation (deg)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##AngleBand", &ap.angleDeadbandInDeg, 0.0f, 5.0f, "%.1f deg" );

            label( "Rate Band", "Angular rate deadband against thruster chattering while holding (deg/s)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##RateBand", &ap.rateDeadbandInDegPerS, 0.0f, 5.0f, "%.1f deg/s" );

            label( "Max Rate", "Max angular rate while rotating towards target (deg/s)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##MaxRate", &ap.maxRateInDegPerS, 1.0f, 90.0f, "%.0f deg/s" );

            label( "Force Weight", "Penalty for net force of thruster combos (0 => ship may drift)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##ForceWeight", &ap.forceWeight, 0.0f, 10.0f, "%.2f" );

            label(
                "Vel. Weight",
                "Weight for thruster combos countering the ship's velocity (velocity hold)\n"
                "Too high values let velocity hold fight the rotation (overshoot)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##VelocityWeight", &ap.velocityWeight, 0.0f, 0.5f, "%.2f" );

            label( "Vel. Band", "Velocity deadband per local axis against chattering while nulling velocity (m/s)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##VelocityBand", &ap.velocityDeadband, 0.0f, 0.2f, "%.2f m/s" );

            label(
                "Switch Weight",
                "Penalty per thruster switched ON compared to previous frame (against chattering)\n"
                "Too high values prevent engaging enough thrusters for braking (overshoot)" );
            ImGui::SetNextItemWidth( -FLT_MIN );
            ImGui::SliderFloat( "##SwitchWeight", &ap.switchWeight, 0.0f, 2.0f, "%.2f" );

            values( "Max Accel", "Max angular acceleration (rad/s^2)", toYpr( ap.accelMax ), "%.2f" );
        }
    }
    ImGui::Separator();
    {
        auto affector      = sdb.state< Physics::Affector::Info >( m_testAffector );
        glm::fvec3& torque = affector->torque;
        glm::fvec3& force  = affector->force;

        torque = glm::fvec3( 0.0f );
        force  = glm::fvec3( 0.0f );

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Torque" );
        ImGui::SameLine( 55.0f );
        ImGui::Button( "+X" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( +1.0, 0.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "-X" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( -1.0, 0.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "+Y" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( 0.0, +1.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "-Y" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( 0.0, -1.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "+Z" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( 0.0, 0.0, +1.0 );
        ImGui::SameLine();
        ImGui::Button( "-Z" );
        if ( ImGui::IsItemHovered() && mouseDown ) torque = glm::fvec3( 0.0, 0.0, -1.0 );

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Force" );
        ImGui::SameLine( 55.0f );
        ImGui::Button( "+X" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( +1.0, 0.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "-X" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( -1.0, 0.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "+Y" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( 0.0, +1.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "-Y" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( 0.0, -1.0, 0.0 );
        ImGui::SameLine();
        ImGui::Button( "+Z" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( 0.0, 0.0, +1.0 );
        ImGui::SameLine();
        ImGui::Button( "-Z" );
        if ( ImGui::IsItemHovered() && mouseDown ) force = glm::fvec3( 0.0, 0.0, -1.0 );

        if ( glm::length( torque ) > 0.001f ) {
            affector->enabled = true;
            torque            = spaceShipMesh->rotation * torque;
        }
        if ( glm::length( force ) > 0.001f ) {
            affector->enabled = true;
            force             = spaceShipMesh->rotation * force;
        }
    }

    ImGui::Separator();
    for ( auto& thruster : m_thrusters ) {
        ImGui::Button( thruster.name.c_str() );

        auto infoMesh = sdb.state< Renderer::Mesh::Info >( thruster.infoMeshHandle );
        auto affector = sdb.state< Physics::Affector::Info >( thruster.affectorHandle );

        // Default is for info mesh to be hidden and affector to be disabled
        infoMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
        affector->enabled = false;

        bool active = activeThrusters.find( thruster.name ) != activeThrusters.end();

        if ( ImGui::IsItemHovered() || active ) {
            infoMesh->flags &= ~Renderer::Mesh::Flag::HIDDEN;
            infoMesh->translation = spaceShipMesh->translation + spaceShipMesh->rotation * thruster.pos;
            infoMesh->rotation =
                spaceShipMesh->rotation * Math::rotateFromTo( glm::fvec3( 0.0f, 1.0f, 0.0f ), -thruster.dir );

            // FIXME: 'rotateFromTo' fails if from == -to
            if ( thruster.name == "Front-Wing-Left.Engine-Back" ) {
                int debug = 1;
            }
        }

        if ( ImGui::IsItemHovered() && mouseDown || active ) {
            affector->enabled       = true;
            affector->forcePosition = thruster.pos;
            affector->force         = spaceShipMesh->rotation * thruster.dir;
        }
    }

    ImGui::End();
}
