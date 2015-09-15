// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.08.2015
// -------------------------------------------------------------------------------------------------

#ifndef LOGGING_HPP
#define LOGGING_HPP

#include "Common.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Logging module
struct Logging
{
    Logging();
    virtual ~Logging();

public:
    enum MsgType
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static void debug(const char *format, ...);

private:
    COMMON_DISABLE_COPY(Logging)
};

#endif
