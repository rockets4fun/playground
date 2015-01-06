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

    newType.objectIdToIdx.resize(maxObjectCount + 1, 0);
    for (int id = 0; id < maxObjectCount + 1; ++id)
    {
        newType.objectIdToIdx[id] = id;
    }

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
        State &existingState = m_states[existingStateId];
        COMMON_ASSERT(existingState.typeId == typeId);
        COMMON_ASSERT(existingState.elemSize == elemSize);
        return existingStateId;
    }
    COMMON_ASSERT(isTypeIdValid(typeId));
    Type &type = m_types[typeId];
    COMMON_ASSERT(type.objectCount == 0);

    State newState;
    newState.name = name;
    newState.id = m_states.size();
    newState.typeId = typeId;
    newState.elemSize = elemSize;

    m_states.push_back(newState);
    type.stateIds.push_back(newState.id);
    m_stateIdsByName[name] = newState.id;

    m_stateValues.resize(newState.id + 1);
    m_stateValues[newState.id].resize(newState.elemSize * (type.maxObjectCount + 1));

    return newState.id;
}

// -------------------------------------------------------------------------------------------------
size_t StateDb::createObject(size_t typeId)
{
    COMMON_ASSERT(isTypeIdValid(typeId));
    Type &type = m_types[typeId];
    if (type.objectCount == type.maxObjectCount)
    {
        // TODO(MARTINMO): Out of type memory error
        return 0;
    }
    size_t newObjectId = ++type.objectCount;
    type.objectIdToIdx[newObjectId] = newObjectId;
    return newObjectId;
}

// -------------------------------------------------------------------------------------------------
void StateDb::deleteObject(size_t typeId, size_t objectId)
{
    COMMON_ASSERT(isTypeIdValid(typeId));
    Type &type = m_types[typeId];
    if (type.objectCount < 1)
    {
        return;
    }
    if (type.objectCount > 1)
    {
        type.objectIdToIdx[type.objectCount] = objectId;
    }
    --type.objectCount;
}

// -------------------------------------------------------------------------------------------------
void *StateDb::state(size_t stateId, size_t objectId)
{
    COMMON_ASSERT(isStateIdValid(stateId));
    State &state = m_states[stateId];
    Type &type = m_types[state.typeId];
    COMMON_ASSERT(objectId <= type.objectCount);
    return &m_stateValues[stateId][type.objectIdToIdx[objectId] * state.elemSize];
}
