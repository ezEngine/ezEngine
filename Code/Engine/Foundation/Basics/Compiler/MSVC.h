#pragma once

#if defined(_MSC_VER) && !defined(__clang__)

#  undef EZ_COMPILER_MSVC
#  define EZ_COMPILER_MSVC EZ_ON

#  if __clang__ || __castxml__
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

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG) || (_MSC_VER >= 1929 /* broken in early VS2019 but works again in VS2022 and later 2019 versions*/)

#    define EZ_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }

#  else

#    define EZ_DEBUG_BREAK                         \
      {                                            \
        /* Declared with DLL export in Assert.h */ \
        MSVC_OutOfLine_DebugBreak();               \
      }

#  endif

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
#    define EZ_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, N, ...) N
#    define EZ_VA_NUM_ARGS_REVERSE_SEQUENCE 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define EZ_LEFT_PARENTHESIS (
#    define EZ_RIGHT_PARENTHESIS )
#    define EZ_VA_NUM_ARGS(...) EZ_VA_NUM_ARGS_HELPER EZ_LEFT_PARENTHESIS __VA_ARGS__, EZ_VA_NUM_ARGS_REVERSE_SEQUENCE EZ_RIGHT_PARENTHESIS
#  endif

#  define EZ_WARNING_PUSH() __pragma(warning(push))
#  define EZ_WARNING_POP() __pragma(warning(pop))
#  define EZ_WARNING_DISABLE_MSVC(_x) __pragma(warning(disable \
                                                       : _x))

#  define EZ_DECL_EXPORT __declspec(dllexport)
#  define EZ_DECL_IMPORT __declspec(dllimport)
#  define EZ_DECL_EXPORT_FRIEND __declspec(dllexport)
#  define EZ_DECL_IMPORT_FRIEND __declspec(dllimport)

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#  define EZ_MSVC_ANALYSIS_WARNING_PUSH __pragma(warning(push))
#  define EZ_MSVC_ANALYSIS_WARNING_POP __pragma(warning(pop))
#  define EZ_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable \
                                                                           : warningNumber))
#  define EZ_MSVC_ANALYSIS_ASSUME(expression) __assume(expression)

#else

#  define EZ_WARNING_DISABLE_MSVC(_x)

/// \brief Define some macros to work with the MSVC analysis warning
/// Note that the StaticAnalysis.h in Basics/Compiler/MSVC will define the MSVC specific versions.
#  define EZ_MSVC_ANALYSIS_WARNING_PUSH
#  define EZ_MSVC_ANALYSIS_WARNING_POP
#  define EZ_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber)
#  define EZ_MSVC_ANALYSIS_ASSUME(expression)

#endif
