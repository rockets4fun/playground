// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 26.07.2017
// -------------------------------------------------------------------------------------------------

#ifndef STR_HPP
#define STR_HPP

#include "Common.hpp"

#include <string>

// -------------------------------------------------------------------------------------------------
/// @brief String routines
struct Str
{
    static bool startsWith( const std::string& str, const std::string& prefix );
    static bool endsWith( const std::string& str, const std::string& suffix );
    static bool fromFile( const std::string& filename, std::string& contents );
    static std::string build( const char* format, ... );

private:
    COMMON_DISABLE_COPY( Str )
};

#endif
