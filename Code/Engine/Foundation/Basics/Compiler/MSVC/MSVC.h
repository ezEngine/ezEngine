#pragma once

#if defined(_MSC_VER) && !defined(__clang__)

#  undef EZ_COMPILER_MSVC
#  define EZ_COMPILER_MSVC EZ_ON

#  ifdef __clang__
#    undef EZ_COMPILER_MSVC_CLANG
#    define EZ_COMPILER_MSVC_CLANG EZ_ON
#  else
#    undef EZ_COMPILER_MSVC_PURE
#    define EZ_COMPILER_MSVC_PURE EZ_ON
#  endif

#  ifdef _DEBUG
#    undef EZ_COMPILE_FOR_DEBUG
#    define EZ_COMPILE_FOR_DEBUG EZ_ON
#  endif


// Functions marked as EZ_ALWAYS_INLINE will be inlined even in Debug builds, which means you will step over them in a debugger
#  define EZ_ALWAYS_INLINE __forceinline

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
#    define EZ_FORCE_INLINE inline
#  else
#    define EZ_FORCE_INLINE __forceinline
#  endif

#  ifdef __INTELLISENSE__
#    define EZ_ALIGN(decl, alignment) decl
#    define EZ_ALIGN_VARIABLE(decl, alignment) decl
#  else
#    define EZ_ALIGN(decl, alignment) __declspec(align(alignment)) decl
#    define EZ_ALIGN_VARIABLE(decl, alignment) __declspec(align(alignment)) decl
#  endif

// workaround for msvc compiler issue with alignment determination of dependent types
#  define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MAX(EZ_ALIGNMENT_MINIMUM, EZ_COMPILE_TIME_MIN(sizeof(type), __alignof(type)))

#  define EZ_DEBUG_BREAK \
    {                    \
      __debugbreak();    \
    }

#  if EZ_ENABLED(EZ_COMPILER_MSVC_CLANG)
#    define EZ_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  else
#    define EZ_SOURCE_FUNCTION __FUNCTION__
#  endif

#  define EZ_SOURCE_LINE __LINE__
#  define EZ_SOURCE_FILE __FILE__

// EZ_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro
// unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
// https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
#  if _MSC_VER >= 1400 && EZ_DISABLED(EZ_COMPILER_MSVC_CLANG)
#    define EZ_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#    define EZ_VA_NUM_ARGS_REVERSE_SEQUENCE 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define EZ_LEFT_PARENTHESIS (
#    define EZ_RIGHT_PARENTHESIS )
#    define EZ_VA_NUM_ARGS(...) EZ_VA_NUM_ARGS_HELPER EZ_LEFT_PARENTHESIS __VA_ARGS__, EZ_VA_NUM_ARGS_REVERSE_SEQUENCE EZ_RIGHT_PARENTHESIS
#  endif

#endif
