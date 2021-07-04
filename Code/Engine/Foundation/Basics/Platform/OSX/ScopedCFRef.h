#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <CoreFoundation/CoreFoundation.h>

/// \brief Helper class to release references of core foundation objects correctly.
template <typename T>
class ezScopedCFRef
{
public:
  ezScopedCFRef(T Ref)
    : m_Ref(Ref)
  {
  }

  ~ezScopedCFRef()
  {
    CFRelease(m_Ref);
  }

  operator T() const
  {
    return m_Ref;
  }

private:
  T m_Ref;
};
