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
/// With this implementation we pay (in terms of run-time) on:
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
    std::string stateNameById(u64 stateId);
    u64 registerState(u64 typeId, const std::string &name, u64 elemSize);

    bool isHandleValid(u64 objectHandle);
    std::string handleTypeName(u64 objectHandle);

    template< class ElementType >
    u64 handleFromState(ElementType *elem)
    {
        COMMON_ASSERT(isStateIdValid(ElementType::STATE));

        const State &state = m_states[ElementType::STATE];
        const Type &type = m_types[state.typeId];

        u64 offsetInB = (unsigned char *)elem - &m_stateValues[ElementType::STATE][0];
        if (offsetInB % state.elemSize != 0)
        {
            return 0;
        }
        if (offsetInB / state.elemSize > type.objectCount)
        {
            return 0;
        }

        u64 objectId = type.idxToObjectId[offsetInB / state.elemSize];
        return composeObjectHandle(u16(state.typeId),
            u16(type.lifecycleByObjectId[objectId]), u32(objectId));
    }

    void destroy(u64 objectHandle);
    int count(u64 typeId);

    template< class ElementType >
    ElementType *create(u64 &createdObjectHandle)
    {
        COMMON_ASSERT(isStateIdValid(ElementType::STATE));
        // TODO(martinmo): Encode type ID into the state ID (==> state handle)
        // TODO(martinmo): ==> This way we can avoid lookup in 'm_states'
        u64 typeId = m_states[ElementType::STATE].typeId;

        COMMON_ASSERT(isTypeIdValid(typeId));

        Type &type = m_types[typeId];
        if (type.objectCount >= type.maxObjectCount)
        {
            Logging::debug("Out of memory for type \"%s\"", type.name.c_str());
            createdObjectHandle = 0;
            return nullptr;
        }

        u64 objectId = type.idxToObjectId[++type.objectCount];
        u64 &lifecycle = type.lifecycleByObjectId[objectId];
        ++lifecycle;

        createdObjectHandle = composeObjectHandle(u16(typeId), u16(lifecycle), u32(objectId));
        return state< ElementType >(createdObjectHandle);
    }

    template< class ElementType >
    ElementType *create()
    {
        u64 ignoreObjectHandle = 0;
        return create< ElementType >(ignoreObjectHandle);
    }

    template< class ElementType >
    ElementType *state(u64 objectHandle)
    {
        COMMON_ASSERT(isStateIdValid(ElementType::STATE));
        COMMON_ASSERT(m_states[ElementType::STATE].elemSize == sizeof(ElementType));

        u64 typeIdFromHandle = objectHandleTypeId(objectHandle);

        COMMON_ASSERT(isHandleValid(objectHandle));
        COMMON_ASSERT(typeIdFromHandle == m_states[ElementType::STATE].typeId);

        // Type lookup needed here for object ID to index translation
        const Type &type = m_types[typeIdFromHandle];
        return (ElementType *)&m_stateValues[ElementType::STATE]
            [type.objectIdToIdx[objectHandle & 0xffffffff] * sizeof(ElementType)];
    }

    template< class ElementType >
    void stateAll(ElementType **begin, ElementType **end = nullptr)
    {
        COMMON_ASSERT(isStateIdValid(ElementType::STATE));
        COMMON_ASSERT(m_states[ElementType::STATE].elemSize == sizeof(ElementType));

        // TODO(martinmo): Encode type ID into the state ID (==> state handle)
        // TODO(martinmo): ==> This way we can avoid lookup in 'm_states'
        // TODO(martinmo): ==> See 'createObjectAndRefState' for more profiteers
        // TODO(martinmo): Store object count in memory block itself
        // TODO(martinmo): ==> This way we can avoid type/implicit state lookup altogether
        const Type &type = m_types[m_states[ElementType::STATE].typeId];

        // TODO(martinmo): Pointers to first and last element never change at runtime ==> store
        std::vector< unsigned char > &memory = m_stateValues[ElementType::STATE];
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
        std::vector< u64 > lifecycleByObjectId;
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
