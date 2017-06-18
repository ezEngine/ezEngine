#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
  #error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
  #undef EZ_PLATFORM_64BIT
  #define EZ_PLATFORM_64BIT EZ_ON
#else
  #undef EZ_PLATFORM_32BIT
  #define EZ_PLATFORM_32BIT EZ_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <winsock2.h>

#include <Windows.h>
#include <Shellapi.h>
#include <Rpc.h>

#include <crtdbg.h>

#include <malloc.h>

# undef EZ_PLATFORM_WINDOWS_UWP
# undef EZ_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
# if WINAPI_FAMILY==WINAPI_FAMILY_APP
#   define EZ_PLATFORM_WINDOWS_UWP EZ_ON
#   define EZ_PLATFORM_WINDOWS_DESKTOP EZ_OFF
# else
#   define EZ_PLATFORM_WINDOWS_UWP EZ_OFF
#   define EZ_PLATFORM_WINDOWS_DESKTOP EZ_ON
# endif

// Windows SDK version. Applies both to classic WinAPI and newer WinRT interfaces.
enum ezWinRTSDKVersion
{
  EZ_WINDOWS_VERSION_7,
  EZ_WINDOWS_VERSION_8,
  EZ_WINDOWS_VERSION_8_1,
  EZ_WINDOWS_VERSION_10_TH1,  // First Windows 10 Release
  EZ_WINDOWS_VERSION_10_TH2,  // The "November Upgrade"
  EZ_WINDOWS_VERSION_10_RS1,  // The "Anniversary Update"
  EZ_WINDOWS_VERSION_10_RS2,  // The "Creator's Update"
  EZ_WINDOWS_VERSION_10_RS3,  // The "Fall Creator's Update"

  // Assume newer version
  EZ_WINDOWS_VERSION_UNKNOWN = 127,
};

// According to Raymond Chen, the NTDDI_VERSION macro was introduced in Windows Vista and is the new standard to determined the supported windows version.
// https://blogs.msdn.microsoft.com/oldnewthing/20070411-00/?p=27283
// However, starting in RS2 NTDDI_VERSION is set to the new WDK_NTDDI_VERSION variable which gives you the SDK version.
// Before that, NTDDI_VERSION gave you only the minimum supported windows version for your current build.
// So in this case we just take a guess by checking which NTDDI variables are there.
#if defined(WDK_NTDDI_VERSION)  // >=RS2
#  if defined(NTDDI_WIN10_RS2) && WDK_NTDDI_VERSION == NTDDI_WIN10_RS2
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_10_RS2
#  elif defined(NTDDI_WIN10_RS3) && WDK_NTDDI_VERSION == NTDDI_WIN10_RS3
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_10_RS3
#  else
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_UNKNOWN
#  endif
#else // <RS2
#  if defined(NTDDI_WIN10_RS1)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_10_RS1
#  elif defined(NTDDI_WIN10_TH2)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_10_TH2
#  elif defined(NTDDI_WIN10_TH1)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_10_TH1
#  elif defined(NTDDI_WINBLUE)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_8_1
#  elif defined(NTDDI_WIN8)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_8
#  elif defined(NTDDI_WIN7)
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_7
#  else
#     define EZ_WINDOWS_SDK_VERSION EZ_WINDOWS_VERSION_UNKNOWN
#  endif
#endif


// unset windows macros
#undef min
#undef max
#undef GetObject
#undef GetCommandLine
#undef ERROR
#undef CreateWindow
#undef DeleteFile
#undef CreateDirectory
#undef CopyFile
#undef DispatchMessage
#undef PostMessage
#undef SendMessage
#undef DrawText

#if defined(_MSC_VER)

  #undef EZ_COMPILER_MSVC
  #define EZ_COMPILER_MSVC EZ_ON

  #ifdef __clang__
    #undef EZ_COMPILER_MSVC_CLANG
    #define EZ_COMPILER_MSVC_CLANG EZ_ON
  #else
    #undef EZ_COMPILER_MSVC_PURE
    #define EZ_COMPILER_MSVC_PURE EZ_ON
  #endif

  #ifdef _DEBUG
    #undef EZ_COMPILE_FOR_DEBUG
    #define EZ_COMPILE_FOR_DEBUG EZ_ON
  #endif

  #define EZ_ANALYSIS_ASSUME(code_to_be_true) __analysis_assume(code_to_be_true)
  #define EZ_ANALYSIS_IGNORE_WARNING_ONCE(x) __pragma(warning(suppress:x))
  #define EZ_ANALYSIS_IGNORE_WARNING_START(x) __pragma(warning(push)) __pragma(warning(disable:x))
  #define EZ_ANALYSIS_IGNORE_WARNING_END __pragma(warning(pop))
  #define EZ_ANALYSIS_IGNORE_ALL_START __pragma(warning(push)) __pragma(warning(disable: 4251 6001 6011 6031 6211 6246 6326 6385 6386 6387 6540))
  #define EZ_ANALYSIS_IGNORE_ALL_END __pragma(warning(pop))

  // On MSVC 2008 in 64 Bit "intin.h" generates a lot of warnings
  #define EZ_MSVC_WARNING_NUMBER 4985
  #include <Foundation/Basics/Compiler/DisableWarning.h>

    EZ_ANALYSIS_IGNORE_WARNING_START(6540)
      #include <intrin.h>
    EZ_ANALYSIS_IGNORE_WARNING_END

  #include <Foundation/Basics/Compiler/RestoreWarning.h>

  // Functions marked as EZ_ALWAYS_INLINE will be inlined even in Debug builds, which means you will step over them in a debugger
  #define EZ_ALWAYS_INLINE __forceinline

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    #define EZ_FORCE_INLINE inline
  #else
    #define EZ_FORCE_INLINE __forceinline
  #endif

  #ifdef __INTELLISENSE__
    #define EZ_ALIGN(decl, alignment) decl
  #else
    #define EZ_ALIGN(decl, alignment) __declspec(align(alignment)) decl
  #endif

  // workaround for msvc compiler issue with alignment determination of dependent types
  #define EZ_ALIGNMENT_OF(type) EZ_COMPILE_TIME_MIN(sizeof(type), __alignof(type))

  #define EZ_DEBUG_BREAK { __debugbreak(); }

  #define EZ_SOURCE_FUNCTION __FUNCTION__
  #define EZ_SOURCE_LINE __LINE__
  #define EZ_SOURCE_FILE __FILE__

  // Set Warnings as Errors: Too few/many parameters given for Macro
  #pragma warning(error : 4002 4003)

  // Enable 'symbol' is not defined as a preprocessor macro
  // #pragma warning(3 : 4668)

  // class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates)
  #pragma warning(disable: 4251)

  // behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
  #pragma warning(disable: 4345)

  // EZ_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro
  // unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
  // https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
  #if _MSC_VER >= 1400 && EZ_DISABLED(EZ_COMPILER_MSVC_CLANG)
    #define EZ_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
    #define EZ_VA_NUM_ARGS_REVERSE_SEQUENCE            10, 9, 8, 7, 6, 5, 4, 3, 2, 1
    #define EZ_LEFT_PARENTHESIS (
    #define EZ_RIGHT_PARENTHESIS )
    #define EZ_VA_NUM_ARGS(...)                        EZ_VA_NUM_ARGS_HELPER EZ_LEFT_PARENTHESIS __VA_ARGS__, EZ_VA_NUM_ARGS_REVERSE_SEQUENCE EZ_RIGHT_PARENTHESIS
  #endif

  #ifndef va_copy
    #define va_copy(dest, source) (dest) = (source)
  #endif

#else
  #error "Unsupported compiler on windows"
#endif

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

