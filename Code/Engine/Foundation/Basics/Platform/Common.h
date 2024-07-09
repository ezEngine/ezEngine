#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_MSVC(4985)

// include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

EZ_WARNING_POP()

// redefine NULL to nullptr
#undef NULL
#define NULL nullptr

// include c++11 specific header
#include <type_traits>
#include <utility>

/// \brief Disallow the copy constructor and the assignment operator for this type.
#define EZ_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;             \
  void operator=(const type&) = delete

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
/// \brief Macro helper to check alignment
#  define EZ_CHECK_ALIGNMENT(ptr, alignment) EZ_ASSERT_DEV(((size_t)ptr & ((alignment) - 1)) == 0, "Wrong alignment.")
#else
/// \brief Macro helper to check alignment
#  define EZ_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define EZ_WINCHECK_1 1          // EZ_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define EZ_WINCHECK_1_WINDOWS_ 1 // EZ_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define EZ_WINCHECK_EZ_INCLUDED_WINDOWS_H \
  0                              // EZ_INCLUDED_WINDOWS_H undefined (stringyfied to "EZ_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define EZ_WINCHECK_EZ_INCLUDED_WINDOWS_H_WINDOWS_ \
  1                              // EZ_INCLUDED_WINDOWS_H undefined (stringyfied to "EZ_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// \brief Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define EZ_CHECK_WINDOWS_INCLUDE(EZ_WINH_INCLUDED, WINH_INCLUDED)                               \
  static_assert(EZ_PP_CONCAT(EZ_WINCHECK_, EZ_PP_CONCAT(EZ_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
    "Windows.h has been included but not through ez. #include <Foundation/Basics/Platform/Win/IncludeWindows.h> instead of Windows.h");

#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of EZ_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define EZ_STATICLINK_FILE(LibraryName, UniqueName) EZ_CHECK_WINDOWS_INCLUDE(EZ_INCLUDED_WINDOWS_H, _WINDOWS_)

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after EZ_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see EZ_STATICLINK_FILE
#  define EZ_STATICLINK_REFERENCE(UniqueName)

/// \brief This must occur exactly once in each static library, such that all EZ_STATICLINK_FILE macros can reference it.
#  define EZ_STATICLINK_LIBRARY(LibraryName) void ezReferenceFunction_##LibraryName(bool bReturn = true)

#else

struct ezStaticLinkHelper
{
  using Func = void (*)(bool);
  ezStaticLinkHelper(Func f) { f(true); }
};

/// \brief Helper struct to register the existence of statically linked plugins.
/// The macro EZ_STATICLINK_LIBRARY will register a the given library name prepended with `ez` to the ezPlugin system.
/// Implemented in Plugin.cpp.
struct EZ_FOUNDATION_DLL ezPluginRegister
{
  ezPluginRegister(const char* szName);
};

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of EZ_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define EZ_STATICLINK_FILE(LibraryName, UniqueName)       \
    extern "C"                                              \
    {                                                       \
      void ezReferenceFunction_##UniqueName(bool bReturn)   \
      {                                                     \
        (void)bReturn;                                      \
      }                                                     \
      void ezReferenceFunction_##LibraryName(bool bReturn); \
    }                                                       \
    static ezStaticLinkHelper StaticLinkHelper_##UniqueName(ezReferenceFunction_##LibraryName);

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after EZ_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see EZ_STATICLINK_FILE
#  define EZ_STATICLINK_REFERENCE(UniqueName)                   \
    void ezReferenceFunction_##UniqueName(bool bReturn = true); \
    ezReferenceFunction_##UniqueName()

/// \brief This must occur exactly once in each static library, such that all EZ_STATICLINK_FILE macros can reference it.
#  define EZ_STATICLINK_LIBRARY(LibraryName)                                                         \
    ezPluginRegister ezPluginRegister_##LibraryName(EZ_PP_STRINGIFY(EZ_PP_CONCAT(ez, LibraryName))); \
    extern "C" void ezReferenceFunction_##LibraryName(bool bReturn = true)

#endif

namespace ezInternal
{
  template <typename T>
  constexpr bool AlwaysFalse = false;

  template <typename T>
  struct ArraySizeHelper
  {
    static_assert(AlwaysFalse<T>, "Cannot take compile time array size of given type");
  };

  template <typename T, size_t N>
  struct ArraySizeHelper<T[N]>
  {
    static constexpr size_t value = N;
  };

} // namespace ezInternal

/// \brief Macro to determine the size of a static array
#define EZ_ARRAY_SIZE(a) (ezInternal::ArraySizeHelper<decltype(a)>::value)

/// \brief Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void EZ_IGNORE_UNUSED(const T&)
{
}

#if (__cplusplus >= 202002L || _MSVC_LANG >= 202002L)
#  undef EZ_USE_CPP20_OPERATORS
#  define EZ_USE_CPP20_OPERATORS EZ_ON
#endif

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)
// in C++ 20 we don't need to declare an operator!=, it is automatically generated from operator==
#  define EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(...) /*empty*/
#else
#  define EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(...)                                   \
    EZ_ALWAYS_INLINE bool operator!=(EZ_EXPAND_ARGS_COMMA(__VA_ARGS__) rhs) const \
    {                                                                             \
      return !(*this == rhs);                                                     \
    }
#endif
