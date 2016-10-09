// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.03.2015
// -------------------------------------------------------------------------------------------------

#include "Math.hpp"

// -------------------------------------------------------------------------------------------------
glm::fquat Math::rotateFromTo(const glm::fvec3 &from, const glm::fvec3 &to)
{
    // FIXME(martinmo): Handle case where 'from' and 'to' are parallel
    // FIXME(martinmo): ==> Cross product not well defined in this case
    glm::fvec3 axis = glm::cross(from, to);
    glm::fquat result = glm::fquat(glm::dot(from, to), axis.x, axis.y, axis.z);
    result.w += glm::length(result);
    return glm::normalize(result);
}

// -------------------------------------------------------------------------------------------------
void Math::decomposeTransform(const glm::fmat4 &xform,
    glm::fvec3 &translation, glm::fquat &rotation, glm::vec3 &scale)
{
    // TODO(MARTINMO): Make sure base is orthogonal and return error otherwise

    // Translation
    {
        translation = glm::fvec3(xform[3]);
    }
    // Rotation
    glm::fmat3 xform3 = glm::fmat3(xform);
    {
        // Normalize X, Y base vectors
        xform3[0] = glm::normalize(xform3[0]);
        xform3[1] = glm::normalize(xform3[1]);
        // Z from cross product to get ortho-normal base
        xform3[2] = glm::cross(xform3[0], xform3[1]);
        rotation = glm::quat_cast(xform3);
    }
    // Scale
    {
        scale.x = glm::length(xform3[0]);
        scale.y = glm::length(xform3[1]);
        scale.z = glm::length(xform3[2])
            * glm::dot(xform3[2], glm::fvec3(xform[2]));
    }
}
