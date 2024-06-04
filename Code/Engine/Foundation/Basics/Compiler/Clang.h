#pragma once

#ifdef __clang__

#  undef EZ_COMPILER_CLANG
#  define EZ_COMPILER_CLANG EZ_ON

#  define EZ_ALWAYS_INLINE __attribute__((always_inline)) inline
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
#    define EZ_FORCE_INLINE inline
#  else
#    define EZ_FORCE_INLINE __attribute__((always_inline)) inline
#  endif

#  define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MAX(__alignof(type), EZ_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define EZ_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif __has_builtin(__debugbreak)
#    define EZ_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
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

#  define EZ_WARNING_PUSH() _Pragma("clang diagnostic push")
#  define EZ_WARNING_POP() _Pragma("clang diagnostic pop")
#  define EZ_WARNING_DISABLE_CLANG(_x) _Pragma(EZ_STRINGIZE(clang diagnostic ignored _x))

#  define EZ_DECL_EXPORT [[gnu::visibility("default")]]
#  define EZ_DECL_IMPORT [[gnu::visibility("default")]]
#  define EZ_DECL_EXPORT_FRIEND
#  define EZ_DECL_IMPORT_FRIEND

#else

#  define EZ_WARNING_DISABLE_CLANG(_x)

#endif
