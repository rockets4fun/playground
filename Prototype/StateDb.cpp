// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "StateDb.hpp"

// -------------------------------------------------------------------------------------------------
StateDb::StateDb()
{
    m_types.push_back(Type());
    m_states.push_back(State());
}

// -------------------------------------------------------------------------------------------------
StateDb::~StateDb()
{
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isTypeIdValid(size_t typeId)
{
    return typeId > 0 && typeId < m_types.size();
}

// -------------------------------------------------------------------------------------------------
size_t StateDb::typeIdByName(const std::string &name)
{
    std::map< std::string, size_t >::const_iterator foundType = m_typeIdsByName.find(name);
    if (foundType == m_typeIdsByName.end())
    {
        return 0;
    }
    return foundType->second;
}

// -------------------------------------------------------------------------------------------------
size_t StateDb::registerType(const std::string &name, size_t maxObjectCount)
{
    COMMON_ASSERT(name.length() > 0);
    size_t existingTypeId = typeIdByName(name);
    if (existingTypeId)
    {
        Type *existingType = &m_types[existingTypeId];
        COMMON_ASSERT(existingType->maxObjectCount == maxObjectCount);
    }

    Type newType;
    newType.name = name;
    newType.id = m_types.size();
    newType.maxObjectCount = maxObjectCount;
    m_types.push_back(newType);

    m_typeIdsByName[name] = newType.id;

    return newType.id;
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isStateIdValid(size_t stateId)
{
    return stateId > 0 && stateId < m_states.size();
}

// -------------------------------------------------------------------------------------------------
size_t StateDb::stateIdByName(const std::string &name)
{
    std::map< std::string, size_t >::const_iterator foundState = m_stateIdsByName.find(name);
    if (foundState == m_stateIdsByName.end())
    {
        return 0;
    }
    return foundState->second;
}

// -------------------------------------------------------------------------------------------------
size_t StateDb::registerState(size_t typeId, const std::string &name, size_t elemSize)
{
    COMMON_ASSERT(name.length() > 0);
    size_t existingStateId = stateIdByName(name);
    if (existingStateId)
    {
        State *existingState = &m_states[existingStateId];
        COMMON_ASSERT(existingState->typeId == typeId);
        COMMON_ASSERT(existingState->elemSize == elemSize);
        return existingStateId;
    }
    COMMON_ASSERT(isTypeIdValid(typeId));
    Type *type = &m_types[typeId];
    COMMON_ASSERT(type->objectCount == 0);

    State newState;
    newState.name = name;
    newState.id = m_states.size();
    newState.elemSize = elemSize;
    m_states.push_back(newState);

    m_stateIdsByName[name] = newState.id;

    m_stateValues.resize(newState.elemSize * type->maxObjectCount);

    return newState.id;
}
