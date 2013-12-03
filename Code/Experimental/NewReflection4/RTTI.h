#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>

// *****************************************
// ***** Runtime Type Information Data *****

class ezRTTI : public ezEnumerable<ezRTTI>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRTTI);

public:
  ezRTTI(const char* szName, const ezRTTI* pParentType);

  const char* GetTypeName() const { return m_szTypeName; }

  const ezRTTI* GetParentType() const { return m_pParentType; }

private:
  const char* m_szTypeName;
  const ezRTTI* m_pParentType;
};

