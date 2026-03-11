#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class EZ_FOUNDATION_DLL ezPlatformDesc : public ezEnumerable<ezPlatformDesc>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezPlatformDesc);

public:
  ezPlatformDesc(const char* szName, const char* szType)
  {
    m_szName = szName;
    m_szType = szType;
  }

  const char* GetName() const
  {
    return m_szName;
  }

  const char* GetType() const
  {
    return m_szType;
  }

  static const ezPlatformDesc& GetThisPlatformDesc()
  {
    return *s_pThisPlatform;
  }

private:
  static const ezPlatformDesc* s_pThisPlatform;

  const char* m_szName;
  const char* m_szType;
};
