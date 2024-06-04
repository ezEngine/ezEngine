#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef EZ_COMPILER_GCC
#  define EZ_COMPILER_GCC EZ_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define EZ_ALWAYS_INLINE inline
#  define EZ_FORCE_INLINE inline

#  define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MAX(__alignof(type), EZ_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define EZ_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif defined(__i386__) || defined(__x86_64__)
#    define EZ_DEBUG_BREAK            \
      {                               \
        __asm__ __volatile__("int3"); \
      }
#  else
#    include <signal.h>
#    if defined(SIGTRAP)
#      define EZ_DEBUG_BREAK \
        {                    \
          raise(SIGTRAP);    \
        }
#    else
#      define EZ_DEBUG_BREAK \
        {                    \
          raise(SIGABRT);    \
        }
#    endif
#  endif

#  define EZ_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define EZ_SOURCE_LINE __LINE__
#  define EZ_SOURCE_FILE __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef EZ_COMPILE_FOR_DEBUG
#    define EZ_COMPILE_FOR_DEBUG EZ_ON
#  endif

#  define EZ_WARNING_PUSH() _Pragma("GCC diagnostic push")
#  define EZ_WARNING_POP() _Pragma("GCC diagnostic pop")
#  define EZ_WARNING_DISABLE_GCC(_x) _Pragma(EZ_PP_STRINGIFY(GCC diagnostic ignored _x))

#  define EZ_DECL_EXPORT [[gnu::visibility("default")]]
#  define EZ_DECL_IMPORT [[gnu::visibility("default")]]
#  define EZ_DECL_EXPORT_FRIEND
#  define EZ_DECL_IMPORT_FRIEND

#else

#  define EZ_WARNING_DISABLE_GCC(_x)

#endif
