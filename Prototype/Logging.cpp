// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.08.2015
// -------------------------------------------------------------------------------------------------

#include "Logging.hpp"

#include <cstdarg>
#include <cstdio>
#include <cctype>

// -------------------------------------------------------------------------------------------------
Logging::Logging()
{
}

// -------------------------------------------------------------------------------------------------
Logging::~Logging()
{
}

// -------------------------------------------------------------------------------------------------
void Logging::debug(const char *format, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int indexOfLastChar = ret - 1;
    while (indexOfLastChar >= 0 && isspace(buffer[indexOfLastChar]))
    {
        buffer[indexOfLastChar--] = 0;
    }

    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
}
