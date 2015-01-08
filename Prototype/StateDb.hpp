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

    void *state(u64 stateId, u64 objectHandle);

    template< class ElementType >
    ElementType *fullState(u64 stateId, ElementType **end)
    {
        COMMON_ASSERT(isStateIdValid(stateId));
        State &state = m_states[stateId];
        Type &type = m_types[state.typeId];
        COMMON_ASSERT(state.elemSize == sizeof(ElementType));
        unsigned char *first = &m_stateValues[stateId][state.elemSize];
        *end = (ElementType *)(first + state.elemSize * type.objectCount);
        return (ElementType *)first;
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
        // TODO(MARTINMO): Add version info here for protocol/struct changes
    };

    std::map< std::string, u64 > m_typeIdsByName;
    std::vector< Type > m_types;

    std::map< std::string, u64 > m_stateIdsByName;
    std::vector< State > m_states;

    std::vector< std::vector< unsigned char > > m_stateValues;

    static u64 composeObjectHandle(u16 typeId, u16 lifecycle, u32 objectId);
    static void decomposeObjectHandle(u64 objectHandle, u16 &typeId, u16 &lifecycle, u32 &objectId);

private:
    COMMON_DISABLE_COPY(StateDb);
};

#endif
