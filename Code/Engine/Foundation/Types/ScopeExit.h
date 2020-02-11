
#pragma once

#include <Foundation/Basics.h>

/// \file

/// \brief Macro to execute a piece of code when the current scope closes.
#define EZ_SCOPE_EXIT(code) auto EZ_CONCAT(scopeExit_, EZ_SOURCE_LINE) = ezMakeScopeExit([&]() { code; })

/// \internal Helper class to implement EZ_SCOPE_EXIT
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


