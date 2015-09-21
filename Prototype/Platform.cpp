// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "Platform.hpp"

#include <sys/types.h>
#include <sys/stat.h>

// -------------------------------------------------------------------------------------------------
Platform::Platform()
{
}

// -------------------------------------------------------------------------------------------------
Platform::~Platform()
{
}

// -------------------------------------------------------------------------------------------------
#ifdef COMMON_WINDOWS
#   define stat _stat
#endif
s64 Platform::fileModificationTime(const std::string &filename)
{
    struct stat status;
    if (stat(filename.c_str(), &status) != 0)
    {
        return -1;
    }
    return status.st_mtime;
}
#ifdef COMMON_WINDOWS
#   undef stat
#endif
