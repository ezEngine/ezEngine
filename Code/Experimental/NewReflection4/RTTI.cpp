#include "RTTI.h"

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType)
{
  m_szTypeName = szName;
  m_pParentType = pParentType;
}
