#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
  #error "This header should only be included on windows platforms
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

#if defined(_MSC_VER)

  #undef EZ_COMPILER_MSVC
  #define EZ_COMPILER_MSVC EZ_ON

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

  #define EZ_FORCE_INLINE __forceinline
  #define EZ_RESTRICT __restrict


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

  // declare platform specific types
  typedef unsigned __int64  ezUInt64;
  typedef __int64           ezInt64;

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
  #if _MSC_VER >= 1400
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

