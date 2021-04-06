
#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef EZ_COMPILER_GCC
#  define EZ_COMPILER_GCC EZ_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define EZ_ALWAYS_INLINE inline
#  define EZ_FORCE_INLINE inline

#  define EZ_ALIGN(decl, alignment) __attribute__((aligned(alignment))) decl
#  define EZ_ALIGN_VARIABLE(decl, alignment) decl __attribute__((aligned(alignment)))
#  define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MAX(__alignof(type), EZ_ALIGNMENT_MINIMUM)

#  define EZ_DEBUG_BREAK \
    {                    \
      __builtin_trap();  \
    }

#  define EZ_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define EZ_SOURCE_LINE __LINE__
#  define EZ_SOURCE_FILE __FILE__

#endif
