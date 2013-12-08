#include "RTTI.h"
#include "DynamicRTTI.h"

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> pProperties)
{
  m_szTypeName = szName;
  m_pParentType = pParentType;
  m_pAllocator = pAllocator;
  m_Properties = pProperties;
}

bool ezRTTI::IsDerivedFrom(const ezRTTI* pBaseType) const
{
  const ezRTTI* pThis = this;

  while (pThis != NULL)
  {
    if (pThis == pBaseType)
      return true;

    pThis = pThis->m_pParentType;
  }

  return false;
}

EZ_BEGIN_REFLECTED_TYPE(ezReflectedClass, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezReflectedClass);