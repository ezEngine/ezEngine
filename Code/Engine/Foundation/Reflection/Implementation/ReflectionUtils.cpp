#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>

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
      static_cast<ezTypedMemberProperty<T>*>(m_pProp)->SetValue(m_pObject, m_pValue->ConvertTo<T>());
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
  if (pProp != nullptr && !pProp->IsReadOnly())
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
    {
      static_cast<ezTypedMemberProperty<const char*>*>(pProp)->SetValue(pObject, value.ConvertTo<ezString>().GetData());
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

#define IF_HANDLE_TYPE(TYPE, FUNC) \
  if (prop->GetPropertyType() == ezGetStaticRTTI<TYPE>()) \
  { \
    const ezTypedMemberProperty<TYPE>* pTyped = static_cast<const ezTypedMemberProperty<TYPE>*>(prop); \
    writer.BeginObject(); \
    writer.AddVariableString("t", prop->GetPropertyType()->GetTypeName()); \
    writer.AddVariableString("n", prop->GetPropertyName()); \
    writer.FUNC("v", pTyped->GetValue(pObject)); \
    writer.EndObject(); \
  } \

static void WriteJSONObject(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject, const char* szObjectName);

static void WriteProperties(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType() != nullptr)
  {
    WriteProperties(writer, pRtti->GetParentType(), pObject);
  }

  auto props = pRtti->GetProperties();

  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    if (props[p]->GetCategory() == ezAbstractProperty::Member)
    {
      const ezAbstractMemberProperty* prop = static_cast<ezAbstractMemberProperty*>(props[p]);

      if (prop->IsReadOnly())
        continue;

      // Florian would be proud of me:

      IF_HANDLE_TYPE(bool,    AddVariableBool)

      else IF_HANDLE_TYPE(ezInt8,  AddVariableInt32)
      else IF_HANDLE_TYPE(ezInt16, AddVariableInt32)
      else IF_HANDLE_TYPE(ezInt32, AddVariableInt32)
      else IF_HANDLE_TYPE(ezInt64, AddVariableInt64)

      else IF_HANDLE_TYPE(ezUInt8,  AddVariableUInt32)
      else IF_HANDLE_TYPE(ezUInt16, AddVariableUInt32)
      else IF_HANDLE_TYPE(ezUInt32, AddVariableUInt32)
      else IF_HANDLE_TYPE(ezUInt64, AddVariableUInt64)

      else IF_HANDLE_TYPE(float,  AddVariableFloat)
      else IF_HANDLE_TYPE(double, AddVariableDouble)

      else IF_HANDLE_TYPE(ezMat3,  AddVariableMat3)
      else IF_HANDLE_TYPE(ezMat4,  AddVariableMat4)
      else IF_HANDLE_TYPE(ezQuat,  AddVariableQuat)

      else IF_HANDLE_TYPE(ezVec2,  AddVariableVec2)
      else IF_HANDLE_TYPE(ezVec3,  AddVariableVec3)
      else IF_HANDLE_TYPE(ezVec4,  AddVariableVec4)

      else IF_HANDLE_TYPE(ezColor,  AddVariableColor)
      else IF_HANDLE_TYPE(ezTime,  AddVariableTime)
      else IF_HANDLE_TYPE(ezConstCharPtr,  AddVariableString)

      else if (prop->GetPropertyType()->GetProperties().GetCount() > 0 && prop->GetPropertyPointer(pObject) != nullptr)
      {
        writer.BeginObject();
        writer.AddVariableString("t", "$s");
        writer.AddVariableString("n", prop->GetPropertyName());

        WriteJSONObject(writer, prop->GetPropertyType(), prop->GetPropertyPointer(pObject), "v");
        
        writer.EndObject();
      }
      else
      {
        // it is probably a read-only property
      }
    }
  }
}

static void WriteJSONObject(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject, const char* szObjectName)
{
  writer.BeginObject(szObjectName);

  if (pRtti != nullptr && pObject != nullptr)
  {
    writer.AddVariableString("t", pRtti->GetTypeName());

    writer.BeginArray("p");

    WriteProperties(writer, pRtti, pObject);

    writer.EndArray();
  }
  else
  {
    writer.AddVariableString("t", "null");
  }

  writer.EndObject();
}

void ezReflectionUtils::WriteObjectToJSON(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pObject, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode)
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(WhitespaceMode);

  WriteJSONObject(writer, pRtti, pObject, nullptr);
}

static void ReadJSONObject(const ezVariantDictionary& root, const ezRTTI* pRtti, void* pObject)
{
  ezVariant* pVal;

  if (pObject == nullptr || !root.TryGetValue("p", pVal) || !pVal->IsA<ezVariantArray>())
    return;

  ezVariantArray va = pVal->ConvertTo<ezVariantArray>();
  
  for (ezUInt32 prop = 0; prop < va.GetCount(); ++prop)
  {
    if (va[prop].GetType() != ezVariant::Type::VariantDictionary)
      continue;

    const ezVariantDictionary obj = va[prop].ConvertTo<ezVariantDictionary>();

    ezVariant* pName;
    if (!obj.TryGetValue("n", pName))
      continue;

    ezVariant* pType;
    if (!obj.TryGetValue("t", pType))
      continue;

    ezVariant* pValue;
    if (!obj.TryGetValue("v", pValue))
      continue;

    ezAbstractProperty* pProperty = pRtti->FindPropertyByName(pName->ConvertTo<ezString>().GetData());

    if (pProperty == nullptr)
      continue;

    if (pProperty->GetCategory() != ezAbstractProperty::PropertyCategory::Member)
      continue;

    const ezString sType = pType->ConvertTo<ezString>();

    ezAbstractMemberProperty* pMember = (ezAbstractMemberProperty*) pProperty;

    if (sType != "$s")
    {
      ezReflectionUtils::SetMemberPropertyValue(pMember, pObject, *pValue);
    }
    else
    {
      if (!pValue->IsA<ezVariantDictionary>())
        continue;

      void* pStruct = pMember->GetPropertyPointer(pObject);

      if (pStruct == nullptr)
        continue;// probably read-only

      ReadJSONObject(pValue->ConvertTo<ezVariantDictionary>(), pMember->GetPropertyType(), pStruct);
    }
  }
}

void ezReflectionUtils::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, const ezRTTI& rtti, void* pObject)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return;

  const ezVariantDictionary& root = reader.GetTopLevelObject();

  ReadJSONObject(root, &rtti, pObject);
}

void* ezReflectionUtils::ReadObjectFromJSON(ezStreamReaderBase& stream, const ezRTTI*& pRtti, TypeAllocator Allocator)
{
  pRtti = nullptr;

  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return nullptr;

  const ezVariantDictionary& root = reader.GetTopLevelObject();

  ezVariant* pVal;

  if (!root.TryGetValue("t", pVal))
    return nullptr;

  const ezString sType = pVal->ConvertTo<ezString>();

  if (sType == "null")
    return nullptr;

  pRtti = ezRTTI::FindTypeByName(sType.GetData());

  if (pRtti == nullptr)
    return nullptr;

  void* pObject = nullptr;

  if (Allocator.IsValid())
  {
    pObject = Allocator(*pRtti);
  }
  else
  {
    if (!pRtti->GetAllocator()->CanAllocate())
      return nullptr;

    pObject = pRtti->GetAllocator()->Allocate();
  }

  ReadJSONObject(root, pRtti, pObject);

  return pObject;
}


