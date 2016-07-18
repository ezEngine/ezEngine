
#pragma once

#include <Foundation/Basics.h>

template <typename T>
struct ezScopeExit
{
  EZ_FORCE_INLINE ezScopeExit(T func) : m_func(func)
  {
  }

  EZ_FORCE_INLINE ~ezScopeExit()
  {
    m_func();
  }

  T m_func;
};

template <typename T>
EZ_FORCE_INLINE ezScopeExit<T> ezMakeScopeExit(T func)
{
  return ezScopeExit<T>(func);
}

#define EZ_SCOPE_EXIT(code) auto EZ_CONCAT(scopeExit_, EZ_SOURCE_LINE) = ezMakeScopeExit([&](){code;})
