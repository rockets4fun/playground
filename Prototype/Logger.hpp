// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.08.2015
// -------------------------------------------------------------------------------------------------

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "Common.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Logger module
struct Logger
{
    Logger();
    virtual ~Logger();

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
    COMMON_DISABLE_COPY(Logger)
};

#endif
