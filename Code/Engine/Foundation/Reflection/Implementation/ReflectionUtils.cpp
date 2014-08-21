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

ezUInt32 ezReflectedClassSerializationContext::GetStoredTypeVersion(const ezRTTI* pRtti) const
{
  auto it = m_Read.Find(pRtti);

  EZ_VERIFY(it.IsValid(), "The version for type '%s' has not (yet) been read from the stream.", pRtti->GetTypeName());

  return it.Value();
}

void ezReflectedClassSerializationContext::WriteRttiVersion(ezStreamWriterBase& stream, const ezRTTI* pRtti)
{
  if (!m_Written.Find(pRtti).IsValid())
  {
    ezUInt8 uiUnknown = 1;

    const ezRTTI* pBaseRtti = pRtti->GetParentType();

    // count the number of not-yet-written base class types
    while (pBaseRtti && !m_Written.Find(pBaseRtti).IsValid())
    {
      ++uiUnknown;
      pBaseRtti = pBaseRtti->GetParentType();
    }

    // write the number of types
    stream << uiUnknown;

    pBaseRtti = pRtti;

    // write all base types
    for (ezUInt32 i = 0; i < uiUnknown; ++i)
    {
      // now we know...
      m_Written.Insert(pBaseRtti);

      stream << pBaseRtti->GetTypeName();
      stream << pBaseRtti->GetTypeVersion();

      pBaseRtti = pBaseRtti->GetParentType();
    }
  }

}

void ezReflectedClassSerializationContext::ReadRttiVersion(ezStreamReaderBase& stream, const ezRTTI* pRtti)
{
  if (!m_Read.Find(pRtti).IsValid())
  {
    ezUInt8 uiUnknown = 0;

    stream >> uiUnknown;

    ezString sTypeName;

    // read all base types
    for (ezUInt32 i = 0; i < uiUnknown; ++i)
    {
      ezUInt32 uiVersion = 0;

      stream >> sTypeName;
      stream >> uiVersion;

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTypeName.GetData());

      EZ_VERIFY(pRtti != nullptr, "File contains unknown RTTI type '%s'", sTypeName.GetData());

      // now we know...
      m_Read[pRtti] = uiVersion;
    }
  }
}

void ezReflectedClassSerializationContext::Write(ezStreamWriterBase& stream, const ezReflectedClass& Type)
{
  WriteRttiVersion(stream, Type.GetDynamicRTTI());

  Type.Serialize(stream, *this);
}

void ezReflectedClassSerializationContext::Read(ezStreamReaderBase& stream, ezReflectedClass& Type)
{
  ReadRttiVersion(stream, Type.GetDynamicRTTI());

  Type.Deserialize(stream, *this);
}