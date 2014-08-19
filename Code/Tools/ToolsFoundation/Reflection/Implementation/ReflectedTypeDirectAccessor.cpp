#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include<Foundation/Reflection/Reflection.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeDirectAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(void* pInstance, const ezRTTI* pRtti)
  : ezIReflectedTypeAccessor(ezReflectedTypeManager::GetTypeHandleByName(pRtti->GetTypeName())),
  m_pRtti(pRtti), m_pInstance(pInstance)
{
}

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(ezReflectedClass* pInstance)
  : ezIReflectedTypeAccessor(ezReflectedTypeManager::GetTypeHandleByName(pInstance->GetDynamicRTTI()->GetTypeName())),
  m_pRtti(pInstance->GetDynamicRTTI()), m_pInstance(pInstance)
{
}

const ezVariant ezReflectedTypeDirectAccessor::GetValue(const ezPropertyPath& path) const
{
  const ezRTTI* pRtti = m_pRtti;
  void* pData = m_pInstance;
  const ezAbstractMemberProperty* pProp = ezToolsReflectionUtils::GetMemberPropertyByPath(pRtti, pData, path);
  if (pProp == nullptr)
    return ezVariant();

  return ezReflectionUtils::GetMemberPropertyValue(pProp, pData);
}

bool ezReflectedTypeDirectAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value)
{
  const ezRTTI* pRtti = m_pRtti;
  void* pData = m_pInstance;
  ezAbstractMemberProperty* pProp = ezToolsReflectionUtils::GetMemberPropertyByPath(pRtti, pData, path);
  if (pProp == nullptr)
    return false;

  ezReflectionUtils::SetMemberPropertyValue(pProp, pData, value);
  return true;
}
