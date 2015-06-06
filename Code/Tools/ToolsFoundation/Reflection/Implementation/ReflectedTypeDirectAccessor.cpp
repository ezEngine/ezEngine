#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include<Foundation/Reflection/Reflection.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeDirectAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(void* pInstance, const ezRTTI* pRtti)
  : ezIReflectedTypeAccessor(pRtti),
  m_pRtti(pRtti), m_pInstance(pInstance)
{
}

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(ezReflectedClass* pInstance)
  : ezIReflectedTypeAccessor(pInstance->GetDynamicRTTI()),
  m_pRtti(pInstance->GetDynamicRTTI()), m_pInstance(pInstance)
{
}

const ezVariant ezReflectedTypeDirectAccessor::GetValue(const ezPropertyPath& path) const
{
  return ezToolsReflectionUtils::GetMemberPropertyValueByPath(m_pRtti, m_pInstance, path);
}

bool ezReflectedTypeDirectAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value)
{
  return ezToolsReflectionUtils::SetMemberPropertyValueByPath(m_pRtti, m_pInstance, path, value);
}
