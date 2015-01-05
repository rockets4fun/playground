// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#include "RocketScience.hpp"

#include "Platform.hpp"
#include "StateDb.hpp"

// -------------------------------------------------------------------------------------------------
RocketScience::RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
RocketScience::~RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(Platform &platform)
{
    m_camera = platform.stateDb.createObject(platform.RendererCamera);
    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(Platform &platform)
{
    platform.stateDb.deleteObject(platform.RendererCamera, m_camera);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, real64 deltaTimeInS)
{
}
