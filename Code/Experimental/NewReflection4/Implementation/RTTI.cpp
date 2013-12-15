#include "RTTI.h"
#include "DynamicRTTI.h"
#include "MessageHandler.h"

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> pProperties, ezArrayPtr<ezAbstractMessageHandler*> pMessageHandlers)
{
  m_szTypeName = szName;
  m_pParentType = pParentType;
  m_pAllocator = pAllocator;
  m_Properties = pProperties;
  m_uiTypeSize = uiTypeSize;
  m_MessageHandlers = pMessageHandlers;
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

bool ezRTTI::HandleMessageOfType(void* pInstance, ezMessageId id, ezMessage* pMsg) const
{
  for (ezUInt32 m = 0; m < m_MessageHandlers.GetCount(); ++m)
  {
    if (m_MessageHandlers[m]->GetMessageTypeID() == id)
    {
      m_MessageHandlers[m]->HandleMessage(pInstance, pMsg);
      return true;
    }
  }

  return false;
}


EZ_BEGIN_REFLECTED_TYPE(ezReflectedClass, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezReflectedClass);