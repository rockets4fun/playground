// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Common.hpp"

#include <string>

// -------------------------------------------------------------------------------------------------
/// @brief Platform abstraction
struct Platform
{
    Platform();
    virtual ~Platform();

    static s64 fileModificationTime(const std::string &filename);

public:

private:
    COMMON_DISABLE_COPY(Platform)
};

#endif
