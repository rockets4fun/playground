// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ROCKETSCIENCE_HPP
#define ROCKETSCIENCE_HPP

#include "Common.hpp"

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Brief implementation module description
struct RocketScience : public ModuleIf
{
    RocketScience();
    virtual ~RocketScience();

public: // Implementation of module interface
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, real64 deltaTimeInS);

private:
    size_t m_camera = 0;

private:
    COMMON_DISABLE_COPY(RocketScience);
};

#endif
