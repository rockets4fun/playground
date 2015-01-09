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
bool StateDb::isTypeIdValid(u64 typeId)
{
    return typeId > 0 && typeId < m_types.size();
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::typeIdByName(const std::string &name)
{
    std::map< std::string, u64 >::const_iterator foundType = m_typeIdsByName.find(name);
    if (foundType == m_typeIdsByName.end())
    {
        return 0;
    }
    return foundType->second;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::registerType(const std::string &name, u64 maxObjectCount)
{
    COMMON_ASSERT(maxObjectCount > 0);
    COMMON_ASSERT(name.length() > 0);
    u64 existingTypeId = typeIdByName(name);
    if (existingTypeId)
    {
        Type *existingType = &m_types[existingTypeId];
        COMMON_ASSERT(existingType->maxObjectCount == maxObjectCount);
    }

    Type newType;
    newType.name = name;

    newType.id = m_types.size();
    // Make sure we can store the type ID in an object objectHandle
    COMMON_ASSERT(newType.id <= 0xffff);

    newType.maxObjectCount = maxObjectCount;
    // Make sure we can store the highest object ID in an object objectHandle
    COMMON_ASSERT(newType.maxObjectCount <= 0xffffffff);

    newType.lifecycleByObjectId.resize(maxObjectCount + 1, 0);
    newType.objectIdToIdx.resize(maxObjectCount + 1, 0);
    newType.idxToObjectId.resize(maxObjectCount + 1, 0);
    for (int id = 0; id < maxObjectCount + 1; ++id)
    {
        newType.objectIdToIdx[id] = id;
        newType.idxToObjectId[id] = id;
    }

    m_types.push_back(newType);
    m_typeIdsByName[name] = newType.id;

    return newType.id;
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isStateIdValid(u64 stateId)
{
    return stateId > 0 && stateId < m_states.size();
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::stateIdByName(const std::string &name)
{
    std::map< std::string, u64 >::const_iterator foundState = m_stateIdsByName.find(name);
    if (foundState == m_stateIdsByName.end())
    {
        return 0;
    }
    return foundState->second;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::registerState(u64 typeId, const std::string &name, u64 elemSize)
{
    // Element size has to be non-zero and a multiple of 4 B (32 bit)
    COMMON_ASSERT(elemSize > 0);
    COMMON_ASSERT(elemSize % 4 == 0);

    COMMON_ASSERT(isTypeIdValid(typeId));
    Type &type = m_types[typeId];
    COMMON_ASSERT(type.objectCount == 0);

    std::string internalName = type.name + "::" + name;
    COMMON_ASSERT(internalName.length() > 0);
    u64 existingStateId = stateIdByName(internalName);
    if (existingStateId)
    {
        State &existingState = m_states[existingStateId];
        COMMON_ASSERT(existingState.typeId == typeId);
        COMMON_ASSERT(existingState.elemSize == elemSize);
        return existingStateId;
    }

    State newState;
    newState.name = internalName;
    newState.id = m_states.size();
    newState.typeId = typeId;
    newState.elemSize = elemSize;

    m_states.push_back(newState);
    type.stateIds.push_back(newState.id);
    m_stateIdsByName[internalName] = newState.id;

    m_stateValues.resize(newState.id + 1);
    m_stateValues[newState.id].resize(newState.elemSize * (type.maxObjectCount + 1));

    return newState.id;
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isObjectHandleValid(u64 objectHandle)
{
    u32 objectId;
    u16 typeId, lifecycle;
    decomposeObjectHandle(objectHandle, typeId, lifecycle, objectId);
    if (!isTypeIdValid(typeId))
    {
        return false;
    }
    Type &type = m_types[typeId];
    if (objectId < 1 || objectId > type.maxObjectCount)
    {
        return false;
    }
    if (type.lifecycleByObjectId[objectId] != lifecycle)
    {
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::createObject(u64 typeId)
{
    COMMON_ASSERT(isTypeIdValid(typeId));
    Type &type = m_types[typeId];
    if (type.objectCount >= type.maxObjectCount)
    {
        // TODO(MARTINMO): Out of type memory error
        return u64();
    }
    u64 objectId = type.idxToObjectId[++type.objectCount];
    ++type.lifecycleByObjectId[objectId];
    return composeObjectHandle(u16(typeId),
        u16(type.lifecycleByObjectId[objectId]), u32(objectId));
}

// -------------------------------------------------------------------------------------------------
void StateDb::destroyObject(u64 objectHandle)
{
    COMMON_ASSERT(isObjectHandleValid(objectHandle));

    u32 objectId;
    u16 typeId, lifecycle;
    decomposeObjectHandle(objectHandle, typeId, lifecycle, objectId);

    Type &type = m_types[typeId];
    if (type.objectCount < 1)
    {
        return;
    }
    if (type.objectCount > 1)
    {
        u64 idxToDestroy = type.objectIdToIdx[objectId];
        u64 objectIdToSwapIn = type.idxToObjectId[type.objectCount];
        for (u64 stateId : type.stateIds)
        {
            State &state = m_states[stateId];
            // Swap in last state to fill hole
            memcpy(
                &m_stateValues[stateId][state.elemSize * idxToDestroy],
                &m_stateValues[stateId][state.elemSize * type.objectCount], state.elemSize);
            // Zero memory of swapped in element
            memset(
                &m_stateValues[stateId][state.elemSize * type.objectCount], 0, state.elemSize);
        }
        std::swap(type.objectIdToIdx[objectIdToSwapIn], type.objectIdToIdx[objectId]);
        std::swap(type.idxToObjectId[type.objectCount], type.idxToObjectId[idxToDestroy]);

        ++type.lifecycleByObjectId[objectId];
        COMMON_ASSERT(lifecycle != type.lifecycleByObjectId[objectId]);
    }
    --type.objectCount;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::objectHandleFromElem(u64 stateId, void *elem)
{
    COMMON_ASSERT(isStateIdValid(stateId));
    State &state = m_states[stateId];
    Type &type = m_types[state.typeId];

    u64 offsetInB = (unsigned char *)elem - &m_stateValues[stateId][0];
    if (offsetInB % state.elemSize != 0)
    {
        return 0;
    }
    if (offsetInB / state.elemSize > type.maxObjectCount)
    {
        return 0;
    }

    u64 objectId = type.idxToObjectId[offsetInB / state.elemSize];
    return composeObjectHandle(state.typeId, type.lifecycleByObjectId[objectId], objectId);
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::composeObjectHandle(u16 typeId, u16 lifecycle, u32 objectId)
{
    return u64(typeId) << 48 | u64(lifecycle) << 32 | objectId;
}

// -------------------------------------------------------------------------------------------------
void StateDb::decomposeObjectHandle(u64 objectHandle, u16 &typeId, u16 &lifecycle, u32 &objectId)
{
    typeId = objectHandle >> 48;
    lifecycle = (objectHandle >> 32) & 0xffff;
    objectId = objectHandle & 0xffffffff;
}
