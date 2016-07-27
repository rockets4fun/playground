// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 26.07.2017
// -------------------------------------------------------------------------------------------------

#include "Str.hpp"

#include <fstream>

#include "Logger.hpp"

// -------------------------------------------------------------------------------------------------
bool Str::endsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.length() > str.length())
    {
        return false;
    }
    std::string::size_type pos = str.rfind(suffix);
    return pos == str.length() - suffix.length();
}

// -------------------------------------------------------------------------------------------------
bool Str::fromFile(const std::string &filename, std::string &contents)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        Logger::debug("ERROR: Failed to load file into string \"%s\"", filename.c_str());
        return false;
    }
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return true;
}
