// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.03.2015
// -------------------------------------------------------------------------------------------------

#include "Math.hpp"

const glm::fvec3 Math::X_AXIS = glm::fvec3( 1.0f, 0.0f, 0.0f );
const glm::fvec3 Math::Y_AXIS = glm::fvec3( 0.0f, 1.0f, 0.0f );
const glm::fvec3 Math::Z_AXIS = glm::fvec3( 0.0f, 0.0f, 1.0f );

// -------------------------------------------------------------------------------------------------
glm::fvec3 Math::perpendicular( const glm::fvec3& v )
{
    float x          = glm::abs( v.x );
    float y          = glm::abs( v.y );
    float z          = glm::abs( v.z );
    glm::fvec3 other = x < y ? ( x < z ? X_AXIS : Z_AXIS ) : ( y < z ? Y_AXIS : Z_AXIS );
    return glm::cross( v, other );
}

// -------------------------------------------------------------------------------------------------
glm::fquat Math::rotateFromTo( const glm::fvec3& from, const glm::fvec3& to )
{
    // from "Real-Time Rendering" equation (4.53)
    float e = glm::dot( from, to );
    if ( e < -0.99f ) {
        // vector 'to' points in opposite direction of 'from'
        return glm::angleAxis( glm::radians( 180.0f ), perpendicular( from ) );
    }
    float t = glm::sqrt( 2.0f * ( 1.0f + e ) );
    return glm::fquat( t * 0.5f, 1.0f / t * glm::cross( from, to ) );
}

// -------------------------------------------------------------------------------------------------
void Math::decomposeTransform(
    const glm::fmat4& xform, glm::fvec3& translation, glm::fquat& rotation, glm::vec3& scale )
{
    // TODO(MARTINMO): Make sure base is orthogonal and return error otherwise

    // Translation
    {
        translation = glm::fvec3( xform[ 3 ] );
    }
    // Rotation
    glm::fmat3 xform3 = glm::fmat3( xform );
    {
        // Normalize X, Y base vectors
        xform3[ 0 ] = glm::normalize( xform3[ 0 ] );
        xform3[ 1 ] = glm::normalize( xform3[ 1 ] );
        // Z from cross product to get ortho-normal base
        xform3[ 2 ] = glm::cross( xform3[ 0 ], xform3[ 1 ] );
        rotation    = glm::quat_cast( xform3 );
    }
    // Scale
    {
        scale.x = glm::length( xform3[ 0 ] );
        scale.y = glm::length( xform3[ 1 ] );
        scale.z = glm::length( xform3[ 2 ] ) * glm::dot( xform3[ 2 ], glm::fvec3( xform[ 2 ] ) );
    }
}
