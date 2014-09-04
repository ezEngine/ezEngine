#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionUtils.h>

namespace
{
  struct GetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      m_Result = static_cast<const ezTypedMemberProperty<T>*>(m_pProp)->GetValue(m_pObject);
    }

    const ezAbstractMemberProperty* m_pProp;
    const void* m_pObject;
    ezVariant m_Result;
  };

  struct SetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      static_cast<ezTypedMemberProperty<T>*>(m_pProp)->SetValue(m_pObject, m_pValue->Get<T>());
    }

    ezAbstractMemberProperty* m_pProp;
    void* m_pObject;
    const ezVariant* m_pValue;
  };
}

ezVariant ezReflectionUtils::GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject)
{
  if (pProp != nullptr)
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
      return static_cast<const ezTypedMemberProperty<const char*>*>(pProp)->GetValue(pObject);

    GetValueFunc func;
    func.m_pProp = pProp;
    func.m_pObject = pObject;

    ezVariant::DispatchTo(func, pProp->GetPropertyType()->GetVariantType());

    return func.m_Result;
  }

  return ezVariant();
}

void ezReflectionUtils::SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
{
  if (pProp != nullptr)
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
    {
      static_cast<ezTypedMemberProperty<const char*>*>(pProp)->SetValue(pObject, value.Get<ezString>().GetData());
      return;
    }

    SetValueFunc func;
    func.m_pProp = pProp;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetPropertyType()->GetVariantType());
  }
}

ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  const ezArrayPtr<ezAbstractProperty*>& props = pRtti->GetProperties();
  if (uiPropertyIndex < props.GetCount())
  {
    ezAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == ezAbstractProperty::Member)
      return static_cast<ezAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, const char* szPropertyName)
{
  if (pRtti == nullptr)
    return nullptr;

  if (ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropertyName))
  {
    if (pProp->GetCategory() == ezAbstractProperty::Member)
      return static_cast<ezAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

