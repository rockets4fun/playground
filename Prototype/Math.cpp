// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.03.2015
// -------------------------------------------------------------------------------------------------

#include "Math.hpp"

// -------------------------------------------------------------------------------------------------
glm::fquat Math::rotateFromTo(const glm::fvec3 &from, const glm::fvec3 &to)
{
    // FIXME(martinmo): Handle case where 'from' and 'to' are parallel
    // FIXME(martinmo): --> Cross product not well defined in this case
    glm::fvec3 axis = glm::cross(from, to);
    glm::fquat result = glm::fquat(glm::dot(from, to), axis.x, axis.y, axis.z);
    result.w += glm::length(result);
    return glm::normalize(result);
}
