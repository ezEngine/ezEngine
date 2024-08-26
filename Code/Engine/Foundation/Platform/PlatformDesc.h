#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class EZ_FOUNDATION_DLL ezPlatformDesc : public ezEnumerable<ezPlatformDesc>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezPlatformDesc);

public:
  ezPlatformDesc(const char* szName)
  {
    m_szName = szName;
  }

  const char* GetName() const
  {
    return m_szName;
  }

  static const ezPlatformDesc& GetThisPlatformDesc()
  {
    return *s_pThisPlatform;
  }

private:
  static const ezPlatformDesc* s_pThisPlatform;

  const char* m_szName;
};
