// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef COMMON_HPP
#define COMMON_HPP

typedef float  real32;
typedef double real64;

#define COMMON_DISABLE_COPY(Class) \
private: \
     Class(const Class &); \
     Class &operator = (const Class &);

#define COMMON_ASSERT(Expr) \
    { if (!(Expr)) *(int *)0 = 0; }

#endif
