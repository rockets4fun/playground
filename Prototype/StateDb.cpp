// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#include "StateDb.hpp"

#include "Logger.hpp"

// -------------------------------------------------------------------------------------------------
StateDb::StateDb()
{
    m_types.push_back( Type() );
    m_states.push_back( State() );
}

// -------------------------------------------------------------------------------------------------
StateDb::~StateDb()
{
    for ( auto& type : m_types ) {
        if ( type.objectCount > 0 ) {
            Logger::debug(
                "WARNING: Detected %d active \"%s\"-objects", type.objectCount, type.name.c_str() );
        }
    }
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isTypeIdValid( u64 typeId )
{
    return typeId > 0 && typeId < m_types.size();
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::typeIdByName( const std::string& name )
{
    std::map< std::string, u64 >::const_iterator foundType = m_typeIdsByName.find( name );
    if ( foundType == m_typeIdsByName.end() ) {
        return 0;
    }
    return foundType->second;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::registerType( const std::string& name, u64 maxObjectCount )
{
    COMMON_ASSERT( maxObjectCount > 0 );
    COMMON_ASSERT( name.length() > 0 );
    u64 existingTypeId = typeIdByName( name );
    if ( existingTypeId ) {
        Type* existingType = &m_types[ existingTypeId ];
        COMMON_ASSERT( existingType->maxObjectCount == maxObjectCount );
    }

    Type newType;
    newType.name = name;

    newType.id = m_types.size();
    // Make sure we can store the type ID in an object handle
    COMMON_ASSERT( newType.id <= 0xffff );

    newType.maxObjectCount = maxObjectCount;
    // Make sure we can store the highest object ID in an object handle
    COMMON_ASSERT( newType.maxObjectCount <= 0xffffffff );

    newType.lifecycleByObjectId.resize( maxObjectCount + 1, 0 );
    newType.objectIdToIdx.resize( maxObjectCount + 1, 0 );
    newType.idxToObjectId.resize( maxObjectCount + 1, 0 );
    for ( int id = 0; id < maxObjectCount + 1; ++id ) {
        newType.objectIdToIdx[ id ] = id;
        newType.idxToObjectId[ id ] = id;
    }

    m_types.push_back( newType );
    m_typeIdsByName[ name ] = newType.id;

    return newType.id;
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isStateIdValid( u64 stateId )
{
    return stateId > 0 && stateId < m_states.size();
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::stateIdByName( const std::string& name )
{
    auto foundState = m_stateIdsByName.find( name );
    if ( foundState == m_stateIdsByName.end() ) {
        return 0;
    }
    return foundState->second;
}

// -------------------------------------------------------------------------------------------------
std::string StateDb::stateNameById( u64 stateId )
{
    COMMON_ASSERT( isStateIdValid( stateId ) );
    return m_states[ stateId ].name;
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::registerState( u64 typeId, const std::string& name, u64 elemSize )
{
    // TODO(martinmo): Implemnt this as template method and use 'std::is_pod()'
    // TODO(martinmo): to make sure state struct contains POD types only

    // Element size has to be non-zero and a multiple of 4 B (32 bit)
    COMMON_ASSERT( elemSize > 0 );
    COMMON_ASSERT( elemSize % 4 == 0 );

    COMMON_ASSERT( isTypeIdValid( typeId ) );
    Type& type = m_types[ typeId ];
    COMMON_ASSERT( type.objectCount == 0 );

    std::string internalName = type.name + "::" + name;
    COMMON_ASSERT( internalName.length() > 0 );
    u64 existingStateId = stateIdByName( internalName );
    if ( existingStateId ) {
        State& existingState = m_states[ existingStateId ];
        COMMON_ASSERT( existingState.typeId == typeId );
        COMMON_ASSERT( existingState.elemSize == elemSize );
        return existingStateId;
    }

    State newState;
    newState.name     = internalName;
    newState.id       = m_states.size();
    newState.typeId   = typeId;
    newState.elemSize = elemSize;

    m_states.push_back( newState );
    type.stateIds.push_back( newState.id );
    m_stateIdsByName[ internalName ] = newState.id;

    m_stateValues.resize( newState.id + 1 );
    // FIXME(martinmo): If we want to support non-0 default values for state fields
    // FIXME(martinmo): we would have to use placement new to construct elements here...
    // FIXME(martinmo): ==> Better 'memcpy()' from default initialized 0-element?
    m_stateValues[ newState.id ].resize( newState.elemSize * ( type.maxObjectCount + 1 ) );

    return newState.id;
}

// -------------------------------------------------------------------------------------------------
bool StateDb::isHandleValid( u64 objectHandle )
{
    u16 typeId = objectHandleTypeId( objectHandle );
    if ( !isTypeIdValid( typeId ) ) {
        return false;
    }
    const Type& type = m_types[ typeId ];
    u32 objectId     = objectHandleObjectId( objectHandle );
    if ( objectId < 1 || objectId > type.maxObjectCount ) {
        return false;
    }
    u16 lifecycle = objectHandleLifecycle( objectHandle );
    if ( type.lifecycleByObjectId[ objectId ] != lifecycle ) {
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
std::string StateDb::handleTypeName( u64 objectHandle )
{
    COMMON_ASSERT( isHandleValid( objectHandle ) );
    return m_types[ objectHandleTypeId( objectHandle ) ].name;
}

// -------------------------------------------------------------------------------------------------
void StateDb::destroy( u64 objectHandle )
{
    COMMON_ASSERT( isHandleValid( objectHandle ) );

    Type& type   = m_types[ objectHandleTypeId( objectHandle ) ];
    u32 objectId = objectHandleObjectId( objectHandle );

    COMMON_ASSERT( m_types[ objectHandleTypeId( objectHandle ) ].objectCount > 0 );

    if ( type.objectCount > 1 ) {
        u64 idxToDestroy     = type.objectIdToIdx[ objectId ];
        u64 objectIdToSwapIn = type.idxToObjectId[ type.objectCount ];
        for ( u64 stateId : type.stateIds ) {
            State& state = m_states[ stateId ];
            // Swap in state memory of last object to fill hole
            memcpy(
                &m_stateValues[ stateId ][ state.elemSize * idxToDestroy ],
                &m_stateValues[ stateId ][ state.elemSize * type.objectCount ], state.elemSize );
            // Zero previous state memory of swapped in object
            // FIXME(martinmo): If we want to support non-0 default values for state fields
            // FIXME(martinmo): we would have to use placement new to reset the element here...
            // FIXME(martinmo): ==> Better 'memcpy()' from default initialized 0-element?
            memset( &m_stateValues[ stateId ][ state.elemSize * type.objectCount ], 0, state.elemSize );
        }
        std::swap( type.objectIdToIdx[ objectIdToSwapIn ], type.objectIdToIdx[ objectId ] );
        std::swap( type.idxToObjectId[ type.objectCount ], type.idxToObjectId[ idxToDestroy ] );
    }
    else {
        // Zero memory of destroyed object's states
        // FIXME(martinmo): If we want to support non-0 default values for state fields
        // FIXME(martinmo): we would have to use placement new to reset the element here...
        // FIXME(martinmo): ==> Better 'memcpy()' from default initialized 0-element?
        for ( u64 stateId : type.stateIds ) {
            State& state = m_states[ stateId ];
            memset( &m_stateValues[ stateId ][ state.elemSize * type.objectCount ], 0, state.elemSize );
        }
    }

    ++type.lifecycleByObjectId[ objectId ];
    COMMON_ASSERT( objectHandleLifecycle( objectHandle ) != type.lifecycleByObjectId[ objectId ] );

    --type.objectCount;
}

// -------------------------------------------------------------------------------------------------
int StateDb::count( u64 typeId )
{
    COMMON_ASSERT( isTypeIdValid( typeId ) );
    return int( m_types[ typeId ].objectCount );
}

// -------------------------------------------------------------------------------------------------
u64 StateDb::composeObjectHandle( u16 typeId, u16 lifecycle, u32 objectId )
{
    return u64( typeId ) << 48 | u64( lifecycle ) << 32 | objectId;
}

// -------------------------------------------------------------------------------------------------
u16 StateDb::objectHandleTypeId( u64 objectHandle )
{
    return objectHandle >> 48 & 0xffff;
}

// -------------------------------------------------------------------------------------------------
u16 StateDb::objectHandleLifecycle( u64 objectHandle )
{
    return ( objectHandle >> 32 ) & 0xffff;
}

// -------------------------------------------------------------------------------------------------
u32 StateDb::objectHandleObjectId( u64 objectHandle )
{
    return objectHandle & 0xffffffff;
}
