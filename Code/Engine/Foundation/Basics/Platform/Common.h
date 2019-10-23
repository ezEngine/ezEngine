#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
#define EZ_MSVC_WARNING_NUMBER 4985
#include <Foundation/Basics/Compiler/DisableWarning.h>

// include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

#include <Foundation/Basics/Compiler/RestoreWarning.h>

// redefine NULL to nullptr
#undef NULL
#define NULL nullptr

// include c++11 specific header
#include <type_traits>

#ifndef __INTELLISENSE__

// Macros to do compile-time checks, such as to ensure sizes of types
// EZ_CHECK_AT_COMPILETIME(exp) : only checks exp
// EZ_CHECK_AT_COMPILETIME_MSG(exp, msg) : checks exp and displays msg
#  define EZ_CHECK_AT_COMPILETIME(exp) \
    static_assert(exp, EZ_STRINGIZE(exp) " is false.");

#  define EZ_CHECK_AT_COMPILETIME_MSG(exp, msg) \
    static_assert(exp, EZ_STRINGIZE(exp) " is false. Message: " msg);

#else

// IntelliSense often isn't smart enough to evaluate these conditions correctly

#  define EZ_CHECK_AT_COMPILETIME(exp)

#  define EZ_CHECK_AT_COMPILETIME_MSG(exp, msg)

#endif

/// \brief Disallow the copy constructor and the assignment operator for this type.
#define EZ_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;             \
  void operator=(const type&) = delete

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
/// \brief Macro helper to check alignment
#  define EZ_CHECK_ALIGNMENT(ptr, alignment) \
    EZ_ASSERT_DEV(((size_t)ptr & (alignment - 1)) == 0, "Wrong alignment.")
#else
/// \brief Macro helper to check alignment
#  define EZ_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define EZ_ALIGN_16(decl) EZ_ALIGN(decl, 16)
#define EZ_ALIGN_32(decl) EZ_ALIGN(decl, 32)
#define EZ_ALIGN_64(decl) EZ_ALIGN(decl, 64)
#define EZ_ALIGN_128(decl) EZ_ALIGN(decl, 128)
#define EZ_CHECK_ALIGNMENT_16(ptr) EZ_CHECK_ALIGNMENT(ptr, 16)
#define EZ_CHECK_ALIGNMENT_32(ptr) EZ_CHECK_ALIGNMENT(ptr, 32)
#define EZ_CHECK_ALIGNMENT_64(ptr) EZ_CHECK_ALIGNMENT(ptr, 64)
#define EZ_CHECK_ALIGNMENT_128(ptr) EZ_CHECK_ALIGNMENT(ptr, 128)

#define EZ_WINCHECK_1 1                              // EZ_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define EZ_WINCHECK_1_WINDOWS_ 1                     // EZ_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define EZ_WINCHECK_EZ_INCLUDED_WINDOWS_H 0          // EZ_INCLUDED_WINDOWS_H undefined (stringyfied to "EZ_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define EZ_WINCHECK_EZ_INCLUDED_WINDOWS_H_WINDOWS_ 1 // EZ_INCLUDED_WINDOWS_H undefined (stringyfied to "EZ_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// \brief Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define EZ_CHECK_WINDOWS_INCLUDE(EZ_WINH_INCLUDED, WINH_INCLUDED)                         \
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_CONCAT(EZ_WINCHECK_, EZ_CONCAT(EZ_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
    "Windows.h has been included but not through ez. #include <Foundation/Basics/Platform/Win/IncludeWindows.h> instead of Windows.h");


/// \brief Define some macros to work with the MSVC analysis warning
/// Note that the StaticAnalysis.h in Basics/Compiler/MSVC will define the MSVC specific versions.
#define EZ_MSVC_ANALYSIS_WARNING_PUSH
#define EZ_MSVC_ANALYSIS_WARNING_POP
#define EZ_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber)
#define EZ_MSVC_ANALYSIS_ASSUME(expression)

#if EZ_ENABLED(EZ_COMPILER_MSVC)
  #include <Basics/Compiler/MSVC/StaticAnalysis.h>
#endif

#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of EZ_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define EZ_STATICLINK_FILE(LibraryName, UniqueName) \
    EZ_CHECK_WINDOWS_INCLUDE(EZ_INCLUDED_WINDOWS_H, _WINDOWS_)


/// \brief Used by the tool 'StaticLinkUtil' to generate the block after EZ_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see EZ_STATICLINK_FILE
#  define EZ_STATICLINK_REFERENCE(UniqueName)

/// \brief This must occur exactly once in each static library, such that all EZ_STATICLINK_FILE macros can reference it.
#  define EZ_STATICLINK_LIBRARY(LibraryName) \
    void ezReferenceFunction_##LibraryName(bool bReturn = true)

#else

struct ezStaticLinkHelper
{
  typedef void (*Func)(bool);
  ezStaticLinkHelper(Func f) { f(true); }
};

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of EZ_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define EZ_STATICLINK_FILE(LibraryName, UniqueName)      \
    void ezReferenceFunction_##UniqueName(bool bReturn) {} \
    void ezReferenceFunction_##LibraryName(bool bReturn);  \
    static ezStaticLinkHelper StaticLinkHelper_##UniqueName(ezReferenceFunction_##LibraryName);

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after EZ_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see EZ_STATICLINK_FILE
#  define EZ_STATICLINK_REFERENCE(UniqueName)                   \
    void ezReferenceFunction_##UniqueName(bool bReturn = true); \
    ezReferenceFunction_##UniqueName()

/// \brief This must occur exactly once in each static library, such that all EZ_STATICLINK_FILE macros can reference it.
#  define EZ_STATICLINK_LIBRARY(LibraryName) \
    void ezReferenceFunction_##LibraryName(bool bReturn = true)

#endif

namespace ezInternal
{
  template <typename T, size_t N>
  char (*ArraySizeHelper(T (&)[N]))[N];
}

/// \brief Macro to determine the size of a static array
#define EZ_ARRAY_SIZE(a) (sizeof(*ezInternal::ArraySizeHelper(a)) + 0)

/// \brief Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void EZ_IGNORE_UNUSED(const T&)
{
}


// Math Debug checks
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

#  undef EZ_MATH_CHECK_FOR_NAN
#  define EZ_MATH_CHECK_FOR_NAN EZ_ON

#endif
