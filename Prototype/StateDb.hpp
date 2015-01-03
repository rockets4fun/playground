// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef STATEDB_HPP
#define STATEDB_HPP

#include "Common.hpp"

#include <vector>
#include <map>
#include <string>

// -------------------------------------------------------------------------------------------------
/// @brief Brief implementation module description
struct StateDb
{
    StateDb();
    virtual ~StateDb();

    bool isTypeIdValid(size_t typeId);
    size_t typeIdByName(const std::string &name);
    size_t registerType(const std::string &name, size_t maxObjectCount = 128ull);

    bool isStateIdValid(size_t stateId);
    size_t stateIdByName(const std::string &name);
    size_t registerState(size_t typeId, const std::string &name, size_t elemSize);

    template< class ElemType >
    size_t registerState(size_t typeId, const std::string &name)
    {
        return registerState(typeId, name, sizeof(ElemType));
    }

private:
    struct Type
    {
        std::string name;
        size_t id;
        size_t maxObjectCount = 0;
        size_t objectCount = 0;
        std::vector< size_t > states;
    };

    struct State
    {
        size_t typeId = 0;
        std::string name;
        size_t id;
        size_t elemSize;
    };

    std::map< std::string, size_t > m_typeIdsByName;
    std::vector< Type > m_types;

    std::map< std::string, size_t > m_stateIdsByName;
    std::vector< State > m_states;

    std::vector< std::vector< unsigned char > > m_stateValues;

private:
    COMMON_DISABLE_COPY(StateDb);
};

#endif
