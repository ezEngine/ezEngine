
#pragma once

#include <Foundation/Basics.h>

/// \file
///
/// Scope exit utilities for RAII-style cleanup operations.

/// \brief Executes code automatically when the current scope closes
///
/// Provides a convenient way to ensure cleanup code runs when leaving a scope,
/// regardless of how the scope is exited (normal return, exception, early return).
/// The code is executed in a destructor, guaranteeing cleanup even during stack unwinding.
///
/// Example usage:
/// ```cpp
/// {
///   FILE* file = fopen("test.txt", "r");
///   EZ_SCOPE_EXIT(if (file) fclose(file););
///   // file will be closed automatically when scope ends
/// }
/// ```
#define EZ_SCOPE_EXIT(code) auto EZ_PP_CONCAT(scopeExit_, EZ_SOURCE_LINE) = ezMakeScopeExit([&]() { code; })

/// \internal Helper class implementing RAII scope exit functionality
///
/// Stores a callable object and executes it in the destructor. Used internally
/// by the EZ_SCOPE_EXIT macro to provide exception-safe cleanup operations.
template <typename T>
struct ezScopeExit
{
  EZ_ALWAYS_INLINE ezScopeExit(T&& func)
    : m_func(std::forward<T>(func))
  {
  }

  EZ_ALWAYS_INLINE ~ezScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement EZ_SCOPE_EXIT
template <typename T>
EZ_ALWAYS_INLINE ezScopeExit<T> ezMakeScopeExit(T&& func)
{
  return ezScopeExit<T>(std::forward<T>(func));
}
