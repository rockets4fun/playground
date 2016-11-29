// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.03.2015
// -------------------------------------------------------------------------------------------------

#ifndef MATH_HPP
#define MATH_HPP

#include "Common.hpp"

#include <glm/gtc/quaternion.hpp>

// -------------------------------------------------------------------------------------------------
/// @brief Math/helper routines
struct Math
{
    static const glm::fvec3 X_AXIS;
    static const glm::fvec3 Y_AXIS;
    static const glm::fvec3 Z_AXIS;

    static glm::fvec3 perpendicular(const glm::fvec3 &v);

    static glm::fquat rotateFromTo(const glm::fvec3 &from, const glm::fvec3 &to);

    static void decomposeTransform(const glm::fmat4 &xform,
        glm::fvec3 &translation, glm::fquat &rotation, glm::vec3 &scale);

private:
    COMMON_DISABLE_COPY(Math)
};

#endif
