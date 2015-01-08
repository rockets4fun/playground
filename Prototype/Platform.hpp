// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Common.hpp"

struct StateDb;
struct Renderer;

// -------------------------------------------------------------------------------------------------
/// @brief Platform abstraction
struct Platform
{
    Platform(StateDb &stateDb, Renderer &renderer);
    virtual ~Platform();

public:
    StateDb &stateDb;
    Renderer &renderer;

private:
    COMMON_DISABLE_COPY(Platform);
};

#endif
