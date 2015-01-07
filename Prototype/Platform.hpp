// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Common.hpp"

struct StateDb;

// -------------------------------------------------------------------------------------------------
/// @brief Platform abstraction
struct Platform
{
    Platform(StateDb &stateDb);
    virtual ~Platform();

public:
    StateDb &stateDb;

private:
    COMMON_DISABLE_COPY(Platform);
};

#endif
