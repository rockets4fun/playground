// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 27.07.2017
// -------------------------------------------------------------------------------------------------

#include "Parser.hpp"

#include "Logger.hpp"

std::vector< Parser::CharType > Parser::s_charTypes;

// -------------------------------------------------------------------------------------------------
void Parser::init( const std::string& data )
{
    if ( s_charTypes.empty() ) {
        s_charTypes.resize( 256, CharType::OTHER );
        s_charTypes[ ' ' ]  = CharType::WHITESPACE;
        s_charTypes[ '\t' ] = CharType::WHITESPACE;
        s_charTypes[ '\r' ] = CharType::EOL;
        s_charTypes[ '\n' ] = CharType::EOL;
    }

    *this = Parser();

    m_begin = data.c_str();
    m_end   = data.c_str() + data.length();
};

// -------------------------------------------------------------------------------------------------
bool Parser::advance()
{
    if ( !m_begin ) {
        Logger::debug( "ERROR: Failed to advance parser (not initialized)" );
        return false;
    }
    if ( isEof() ) {
        Logger::debug( "ERROR: Failed to advance parser (at <eof>)" );
        return false;
    }

    if ( !m_tokenBegin )
        m_tokenBegin = m_begin;
    else
        m_tokenBegin = m_tokenEnd;

    while ( s_charTypes[ *m_tokenBegin ] == CharType::WHITESPACE && m_tokenBegin < m_end )
        ++m_tokenBegin;

    m_tokenEnd      = m_tokenBegin;
    m_tokenCharType = s_charTypes[ *m_tokenBegin ];
    while ( s_charTypes[ *m_tokenEnd ] == m_tokenCharType && m_tokenEnd < m_end )
        ++m_tokenEnd;

    //Logger::debug("@ %s", isEof() ? "<eof>" : isEol() ? "<eol>" : str().c_str());

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Parser::isEol()
{
    return m_tokenCharType == CharType::EOL;
}

// -------------------------------------------------------------------------------------------------
bool Parser::isEof()
{
    return m_tokenEnd == m_end;
}

// -------------------------------------------------------------------------------------------------
char Parser::chr()
{
    return *m_tokenBegin;
}

// -------------------------------------------------------------------------------------------------
std::string Parser::str()
{
    return std::string( m_tokenBegin, m_tokenEnd - m_tokenBegin );
}

// -------------------------------------------------------------------------------------------------
bool Parser::uint32( u32& value )
{
    u32 result         = 0;
    u32 multiplier     = 1;
    const char* cursor = m_tokenEnd - 1;
    while ( cursor >= m_tokenBegin ) {
        if ( *cursor < '0' || *cursor > '9' ) {
            return false;
        }
        result += ( *cursor - '0' ) * multiplier;
        multiplier *= 10;
        --cursor;
    }
    value = result;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool Parser::uint32Hex( u32& value )
{
    u64 size = m_tokenEnd - m_tokenBegin;
    if ( size != 8 ) return false;
    u32 result = 0;
    while ( size > 0 ) {
        u8 nibble = *( m_tokenBegin + ( 8 - size ) );
        if ( nibble >= '0' && nibble <= '9' )
            nibble = nibble - '0';
        else if ( nibble >= 'a' && nibble <= 'f' )
            nibble = nibble - 'a' + 10;
        else
            return false;
        result = ( result << 4 ) | nibble;
        --size;
    }
    value = result;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool Parser::floatHex( float& value )
{
    u32 valueInt = 0;
    if ( !uint32Hex( valueInt ) ) {
        return false;
    }
    value = *(float*)&valueInt;
    return true;
}
