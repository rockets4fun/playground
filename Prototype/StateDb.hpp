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
/// @brief State database implementation
///
/// This class implements a state database that ensures that all states of a specific type
/// are always stored tightly packed in memory (think loose structure of arrays).
///
/// Also this class adds means of introspection by storing metadata on state registration.
///
/// Handles are like pointers but they follow an object upon relocation in memory.
///
/// With this implementation we pay on:
/// - Object deletion (fill hole by moving in state from end of state vector)
/// - Object lookup through handle (ID to index traslation via per-type vector)
struct StateDb
{
    StateDb();
    virtual ~StateDb();

    bool isTypeIdValid(u64 typeId);
    u64 typeIdByName(const std::string &name);
    u64 registerType(const std::string &name, u64 maxObjectCount = 128ull);

    bool isStateIdValid(u64 stateId);
    u64 stateIdByName(const std::string &name);
    u64 registerState(u64 typeId, const std::string &name, u64 elemSize);

    bool isObjectHandleValid(u64 objectHandle);

    u64 createObject(u64 typeId);
    void destroyObject(u64 objectHandle);

    int objectCount(u64 typeId);
    u64 objectHandleFromElem(u64 stateId, void *elem);

    template< class ElementType >
    u64 createObjectAndRefState(u64 stateId, ElementType **elem)
    {
        COMMON_ASSERT(isStateIdValid(stateId));

        // TODO(martinmo): Encode type ID into the state ID (==> state handle)
        // TODO(martinmo): ==> This way we can avoid lookup in 'm_states'
        const State &state = m_states[stateId];

        u64 objectHandle = createObject(state.typeId);
        refState(stateId, objectHandle, elem);
        return objectHandle;
    }

    template< class ElementType >
    void refState(u64 stateId, u64 objectHandle, ElementType **elem)
    {
        COMMON_ASSERT(isStateIdValid(stateId));
        COMMON_ASSERT(m_states[stateId].elemSize == sizeof(ElementType));

        u64 typeIdFromHandle = objectHandleTypeId(objectHandle);

        COMMON_ASSERT(isObjectHandleValid(objectHandle));
        COMMON_ASSERT(typeIdFromHandle == m_states[stateId].typeId);

        // Type lookup needed here for object ID to index translation
        const Type &type = m_types[typeIdFromHandle];
        *elem = (ElementType *)&m_stateValues[stateId][
            type.objectIdToIdx[objectHandle & 0xffffffff] * sizeof(ElementType)];
    }

    template< class ElementType >
    ElementType *ref(u64 objectHandle)
    {
        ElementType *result = nullptr;
        refState(ElementType::STATE, objectHandle, &result);
        return result;
    }

    template< class ElementType >
    void refStateAll(u64 stateId, ElementType **begin, ElementType **end = nullptr)
    {
        COMMON_ASSERT(isStateIdValid(stateId));
        COMMON_ASSERT(m_states[stateId].elemSize == sizeof(ElementType));

        // TODO(martinmo): Encode type ID into the state ID (==> state handle)
        // TODO(martinmo): ==> This way we can avoid lookup in 'm_states'
        // TODO(martinmo): ==> See 'createObjectAndRefState' for more profiteers
        // TODO(martinmo): Store object count in memory block itself
        // TODO(martinmo): ==> This way we can avoid type/implicit state lookup altogether
        const Type &type = m_types[m_states[stateId].typeId];

        std::vector< unsigned char > &memory = m_stateValues[stateId];
        unsigned char *memoryBegin = &memory[sizeof(ElementType)];
        *begin = (ElementType *)memoryBegin;
        if (end)
        {
            *end = (ElementType *)(memoryBegin + sizeof(ElementType) * type.objectCount);
        }
    }

private:
    struct Type
    {
        std::string name;
        u64 id;
        u64 maxObjectCount = 0;
        u64 objectCount = 0;
        std::vector< u64 > stateIds;
        std::vector< u64 > objectIdToIdx;
        std::vector< u64 > idxToObjectId;
        std::vector< u16 > lifecycleByObjectId;
    };

    struct State
    {
        u64 typeId = 0;
        std::string name;
        u64 id;
        u64 elemSize;
        // TODO(martinmo): Add version info here for protocol/struct changes
    };

    std::map< std::string, u64 > m_typeIdsByName;
    std::vector< Type > m_types;

    std::map< std::string, u64 > m_stateIdsByName;
    std::vector< State > m_states;

    std::vector< std::vector< unsigned char > > m_stateValues;

    static u64 composeObjectHandle(u16 typeId, u16 lifecycle, u32 objectId);
    static u16 objectHandleTypeId(u64 objectHandle);
    static u16 objectHandleLifecycle(u64 objectHandle);
    static u32 objectHandleObjectId(u64 objectHandle);

private:
    COMMON_DISABLE_COPY(StateDb);
};

#endif
