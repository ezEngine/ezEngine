#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
#  error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
#  undef EZ_PLATFORM_64BIT
#  define EZ_PLATFORM_64BIT EZ_ON
#else
#  undef EZ_PLATFORM_32BIT
#  define EZ_PLATFORM_32BIT EZ_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

//#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <winapifamily.h>

#undef EZ_PLATFORM_WINDOWS_UWP
#undef EZ_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#  define EZ_PLATFORM_WINDOWS_UWP EZ_ON
#  define EZ_PLATFORM_WINDOWS_DESKTOP EZ_OFF
#else
#  define EZ_PLATFORM_WINDOWS_UWP EZ_OFF
#  define EZ_PLATFORM_WINDOWS_DESKTOP EZ_ON
#endif

#ifndef NULL
#define NULL 0
#endif

#if defined(_MSC_VER)

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
#  else
#    define EZ_ALIGN(decl, alignment) __declspec(align(alignment)) decl
#  endif

// workaround for msvc compiler issue with alignment determination of dependent types
#  define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MIN(sizeof(type), __alignof(type))

#  define EZ_DEBUG_BREAK                                                                                                                   \
    {                                                                                                                                      \
      __debugbreak();                                                                                                                      \
    }

#  if EZ_ENABLED(EZ_COMPILER_MSVC_CLANG)
#    define EZ_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  else
#    define EZ_SOURCE_FUNCTION __FUNCTION__
#  endif

#  define EZ_SOURCE_LINE __LINE__
#  define EZ_SOURCE_FILE __FILE__

// Set Warnings as Errors: Too few/many parameters given for Macro
#  pragma warning(error : 4002 4003)

// Enable 'symbol' is not defined as a preprocessor macro
// #pragma warning(3 : 4668)

// class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates)
#  pragma warning(disable : 4251)

// behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
#  pragma warning(disable : 4345)

// nonstandard extension used: nameless struct/union
#  pragma warning(disable : 4201)

// structure was padded due to alignment specifier
#  pragma warning(disable : 4324)

// unreferenced formal parameter
#  pragma warning(disable : 4100)

// local variable is initialized but not referenced
#  pragma warning(disable : 4189)

// conditional expression is constant
#  pragma warning(disable : 4127)

// conversion from 'int 32' to 'int 16', possible loss of data
#  pragma warning(disable : 4244)

// signed/unsigned mismatch
#  pragma warning(disable : 4245)

// signed/unsigned mismatch
#  pragma warning(disable : 4389)

// declaration of 'X' hides previous local declaration
#  pragma warning(disable : 4456)

// cast truncates constant value
#  pragma warning(disable : 4310)

// unreachable code
#  pragma warning(disable : 4702)



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

#  if _M_ARM
#    undef EZ_PLATFORM_ARCH_ARM
#    define EZ_PLATFORM_ARCH_ARM EZ_ON
#  else
#    undef EZ_PLATFORM_ARCH_X86
#    define EZ_PLATFORM_ARCH_X86 EZ_ON
#  endif

#else
#  error "Unsupported compiler on windows"
#endif

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

