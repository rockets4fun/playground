// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 27.07.2017
// -------------------------------------------------------------------------------------------------

#ifndef PARSER_HPP
#define PARSER_HPP

#include "Common.hpp"

#include <string>
#include <vector>

// -------------------------------------------------------------------------------------------------
/// @brief Simple string parsing class
struct Parser
{
    void init( const std::string& data );

    bool advance();

    bool isEol();
    bool isEof();

    char chr();
    std::string str();

    bool uint32( u32& value );
    bool uint32Hex( u32& value );

    bool floatHex( float& value );

private:
    enum CharType
    {
        NONE = 0,
        WHITESPACE,
        EOL,
        OTHER
    };

    static std::vector< CharType > s_charTypes;

    const char* m_begin = nullptr;
    const char* m_end   = nullptr;

    const char* m_tokenBegin = nullptr;
    const char* m_tokenEnd   = nullptr;
    CharType m_tokenCharType = CharType::NONE;
};

#endif
