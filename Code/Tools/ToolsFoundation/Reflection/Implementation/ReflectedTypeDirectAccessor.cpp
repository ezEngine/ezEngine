#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include<Foundation/Reflection/Reflection.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeDirectAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(void* pInstance, const ezRTTI* pRtti, ezDocumentObjectBase* pOwner)
  : ezIReflectedTypeAccessor(pRtti, pOwner),
  m_pRtti(pRtti), m_pInstance(pInstance)
{

}

ezReflectedTypeDirectAccessor::ezReflectedTypeDirectAccessor(ezReflectedClass* pInstance, ezDocumentObjectBase* pOwner)
  : ezIReflectedTypeAccessor(pInstance->GetDynamicRTTI(), pOwner),
  m_pRtti(pInstance->GetDynamicRTTI()), m_pInstance(pInstance)
{
}

const ezVariant ezReflectedTypeDirectAccessor::GetValue(const ezPropertyPath& path, ezVariant index) const
{
  const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
  EZ_ASSERT_DEV(pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Bitflags | ezPropertyFlags::IsEnum), "ezReflectedTypeDirectAccessor: only pod types are supported!");
  return ezToolsReflectionUtils::GetMemberPropertyValueByPath(m_pRtti, m_pInstance, path);
}

bool ezReflectedTypeDirectAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index)
{
  return ezToolsReflectionUtils::SetMemberPropertyValueByPath(m_pRtti, m_pInstance, path, value);
}

ezInt32 ezReflectedTypeDirectAccessor::GetCount(const ezPropertyPath& path) const
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return 0;
}

bool ezReflectedTypeDirectAccessor::GetKeys(const ezPropertyPath & path, ezHybridArray<ezVariant, 16>& out_keys) const
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return false;
}

bool ezReflectedTypeDirectAccessor::InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value)
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return false;
}

bool ezReflectedTypeDirectAccessor::RemoveValue(const ezPropertyPath& path, ezVariant index)
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return false;
}

bool ezReflectedTypeDirectAccessor::MoveValue(const ezPropertyPath & path, ezVariant oldIndex, ezVariant newIndex)
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return false;
}

ezVariant ezReflectedTypeDirectAccessor::GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const
{
  EZ_REPORT_FAILURE("ezReflectedTypeDirectAccessor: Containers are not supported!");
  return ezVariant();
}


