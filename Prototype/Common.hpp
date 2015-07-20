// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 01.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef COMMON_HPP
#define COMMON_HPP

typedef unsigned short int u16;
typedef signed   short int s16;

typedef unsigned long int u32;
typedef signed   long int s32;

typedef unsigned long long int u64;
typedef signed   long long int s64;

#define COMMON_DEBUG

#define COMMON_DISABLE_COPY(Class) \
private: \
     Class(const Class &); \
     Class &operator = (const Class &);

#define COMMON_ASSERT(Expr) \
    { if (!(Expr)) *(int *)0 = 0; }

#endif
