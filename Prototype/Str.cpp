// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 26.07.2017
// -------------------------------------------------------------------------------------------------

#include "Str.hpp"

#include <fstream>
#include <stdarg.h>

#include "Logger.hpp"

// -------------------------------------------------------------------------------------------------
bool Str::startsWith( const std::string& str, const std::string& prefix )
{
    if ( prefix.length() > str.length() ) {
        return false;
    }
    std::string::size_type pos = str.find( prefix );
    return pos == 0;
}

// -------------------------------------------------------------------------------------------------
bool Str::endsWith( const std::string& str, const std::string& suffix )
{
    if ( suffix.length() > str.length() ) {
        return false;
    }
    std::string::size_type pos = str.rfind( suffix );
    return pos == str.length() - suffix.length();
}

// -------------------------------------------------------------------------------------------------
bool Str::fromFile( const std::string& filename, std::string& contents )
{
    std::ifstream in( filename.c_str(), std::ios::in | std::ios::binary );
    if ( !in ) {
        Logger::debug( "ERROR: Failed to load file into string \"%s\"", filename.c_str() );
        return false;
    }
    in.seekg( 0, std::ios::end );
    contents.resize( in.tellg() );
    in.seekg( 0, std::ios::beg );
    in.read( &contents[ 0 ], contents.size() );
    in.close();
    return true;
}

// -------------------------------------------------------------------------------------------------
std::string Str::build( const char* format, ... )
{
    char buffer[ 1024 ];
    va_list args;
    va_start( args, format );
    int ret = vsnprintf( buffer, sizeof( buffer ), format, args );
    va_end( args );
    return std::string( buffer );
}
