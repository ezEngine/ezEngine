#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionSerializer.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>



static void WriteJSONObject(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject, const char* szObjectName);

void ezReflectionSerializer::WritePropertyToJSON(ezJSONWriter& writer, const ezAbstractProperty* pProp, const ezRTTI* pRtti, const void* pObject)
{
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      const ezAbstractMemberProperty* prop = static_cast<const ezAbstractMemberProperty*>(pProp);
      const ezRTTI* pPropRtti = prop->GetPropertyType();

      if (prop->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
        return;

      if (pPropRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
      {
        const ezAbstractEnumerationProperty* pEnumProp = static_cast<const ezAbstractEnumerationProperty*>(prop);
        writer.BeginObject();
        writer.AddVariableString("t", pPropRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum) ? "ezEnum" : "ezBitflags");
        writer.AddVariableString("n", prop->GetPropertyName());
        writer.AddVariableInt64("v", pEnumProp->GetValue(pObject));
        writer.EndObject();
      }
      else if (ezReflectionUtils::IsBasicType(pPropRtti) || pPropRtti == ezGetStaticRTTI<ezVariant>())
      {
        ezVariant value = ezReflectionUtils::GetMemberPropertyValue(prop, pObject);
        writer.BeginObject();
        writer.AddVariableString("t", prop->GetPropertyType()->GetTypeName());
        writer.AddVariableString("n", prop->GetPropertyName());
        writer.AddVariableVariant("v", value);
        writer.EndObject();
      }
      else if (pPropRtti->GetProperties().GetCount() > 0)
      {
        // Do we have direct access to the property?
        if (prop->GetPropertyPointer(pObject) != nullptr)
        {
          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          writer.AddVariableString("n", prop->GetPropertyName());

          WriteJSONObject(writer, pPropRtti, prop->GetPropertyPointer(pObject), "v");

          writer.EndObject();
        }
        // If the property is behind an accessor, we need to retrieve it first.
        else if (pPropRtti->GetAllocator()->CanAllocate())
        {
          void* pTempObject = pPropRtti->GetAllocator()->Allocate();
          prop->GetValuePtr(pObject, pTempObject);

          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          writer.AddVariableString("n", prop->GetPropertyName());

          WriteJSONObject(writer, pPropRtti, pTempObject, "v");

          writer.EndObject();
          pPropRtti->GetAllocator()->Deallocate(pTempObject);
        }
      }
    }
    break;
  case ezPropertyCategory::Array:
    {
      const ezAbstractArrayProperty* prop = static_cast<const ezAbstractArrayProperty*>(pProp);
      const ezRTTI* pElemRtti = prop->GetElementType();

      if (prop->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
        return;

      writer.BeginObject();
      writer.AddVariableString("t", "$array");
      writer.AddVariableString("n", prop->GetPropertyName());
      writer.BeginArray("v");

      ezUInt32 uiCount = prop->GetCount(pObject);
      if (ezReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == ezGetStaticRTTI<ezVariant>())
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          writer.BeginObject();
          writer.AddVariableString("t", pElemRtti->GetTypeName());
          ezVariant value = ezReflectionUtils::GetArrayPropertyValue(prop, pObject, i);
          writer.AddVariableVariant("v", value);
          writer.EndObject();
        }
      }
      else if (pElemRtti->GetAllocator()->CanAllocate())
      {
        void* pTempObject = pElemRtti->GetAllocator()->Allocate();
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          prop->GetValue(pObject, i, pTempObject);
          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          WriteJSONObject(writer, pElemRtti, pTempObject, "v");
          writer.EndObject();
        }
        pElemRtti->GetAllocator()->Deallocate(pTempObject);
      }

      writer.EndArray();
      writer.EndObject();
    }
    break;
  case ezPropertyCategory::Set:
    {
      const ezAbstractSetProperty* prop = static_cast<const ezAbstractSetProperty*>(pProp);
      const ezRTTI* pElemRtti = prop->GetElementType();

      if (prop->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
        return;

      writer.BeginObject();
      writer.AddVariableString("t", "$set");
      writer.AddVariableString("n", prop->GetPropertyName());
      writer.BeginArray("v");

      if (ezReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == ezGetStaticRTTI<ezVariant>())
      {
        ezHybridArray<ezVariant, 16> keys;
        prop->GetValues(pObject, keys);
        for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          writer.BeginObject();
          writer.AddVariableString("t", pElemRtti->GetTypeName());
          writer.AddVariableVariant("v", keys[i]);
          writer.EndObject();
        }
      }
      else
      {
        // TODO pointer type
        EZ_ASSERT_NOT_IMPLEMENTED;
      }

      writer.EndArray();
      writer.EndObject();
    }
    break;
  default:
    break;
  }
}

static void ReadJSONObject(const ezVariantDictionary& root, const ezRTTI* pRtti, void* pObject);

void ezReflectionSerializer::ReadPropertyFromJSON(const ezVariantDictionary& prop, const ezRTTI* pRtti, void* pObject)
{
  ezVariant* pName;
  if (!prop.TryGetValue("n", pName))
    return;

  ezVariant* pType;
  if (!prop.TryGetValue("t", pType))
    return;

  ezVariant* pValue;
  if (!prop.TryGetValue("v", pValue))
    return;

  ezAbstractProperty* pProperty = pRtti->FindPropertyByName(pName->ConvertTo<ezString>().GetData());

  if (pProperty == nullptr)
    return;


  const ezString sType = pType->ConvertTo<ezString>();

  if (sType == "$s") // struct property
  {
    if (pProperty->GetCategory() != ezPropertyCategory::Member)
      return;

    ezAbstractMemberProperty* pMember = (ezAbstractMemberProperty*) pProperty;
    const ezRTTI* pPropRtti = pMember->GetPropertyType();

    if (!pValue->IsA<ezVariantDictionary>())
      return;

    void* pStruct = pMember->GetPropertyPointer(pObject);

    if (pStruct != nullptr)
    {
      ReadJSONObject(pValue->Get<ezVariantDictionary>(), pPropRtti, pStruct);
    }
    else if (pPropRtti->GetAllocator()->CanAllocate())
    {
      void* pTempObject = pPropRtti->GetAllocator()->Allocate();
      ReadJSONObject(pValue->Get<ezVariantDictionary>(), pPropRtti, pTempObject);
      pMember->SetValuePtr(pObject, pTempObject);
      pPropRtti->GetAllocator()->Deallocate(pTempObject);
    }
  }
  else if (sType == "$array") // array property
  {
    if (pProperty->GetCategory() != ezPropertyCategory::Array)
      return;

    if (!pValue->IsA<ezVariantArray>())
      return;

    const ezVariantArray& data = pValue->Get<ezVariantArray>();
    ezAbstractArrayProperty* pArray = (ezAbstractArrayProperty*) pProperty;
    const ezRTTI* pElemRtti = pArray->GetElementType();

    pArray->SetCount(pObject, data.GetCount());
    for (ezUInt32 i = 0; i < data.GetCount(); i++)
    {
      if (!data[i].IsA<ezVariantDictionary>())
        return;
      const ezVariantDictionary& elem = data[i].Get<ezVariantDictionary>();

      ezVariant* pElemType;
      if (!elem.TryGetValue("t", pElemType))
        return;

      ezVariant* pElemValue;
      if (!elem.TryGetValue("v", pElemValue))
        return;

      const ezString sElemType = pElemType->ConvertTo<ezString>();
      if (sElemType == "$s") // struct element
      {
        if (!pElemValue->IsA<ezVariantDictionary>())
          return;

        void* pTempObject = pElemRtti->GetAllocator()->Allocate();
        ReadJSONObject(pElemValue->Get<ezVariantDictionary>(), pElemRtti, pTempObject);
        pArray->SetValue(pObject, i, pTempObject);
        pElemRtti->GetAllocator()->Deallocate(pTempObject);
      }
      else // pod element
      {
        ezReflectionUtils::SetArrayPropertyValue(pArray, pObject, i, *pElemValue);
      }
    }
  }
  else if (sType == "$set") // set property
  {
    if (pProperty->GetCategory() != ezPropertyCategory::Set)
      return;

    if (!pValue->IsA<ezVariantArray>())
      return;

    const ezVariantArray& data = pValue->Get<ezVariantArray>();
    ezAbstractSetProperty* pSet = (ezAbstractSetProperty*) pProperty;
    const ezRTTI* pElemRtti = pSet->GetElementType();

    pSet->Clear(pObject);
    for (ezUInt32 i = 0; i < data.GetCount(); i++)
    {
      if (!data[i].IsA<ezVariantDictionary>())
        return;
      const ezVariantDictionary& elem = data[i].Get<ezVariantDictionary>();

      ezVariant* pElemType;
      if (!elem.TryGetValue("t", pElemType))
        return;

      ezVariant* pElemValue;
      if (!elem.TryGetValue("v", pElemValue))
        return;

      const ezString sElemType = pElemType->ConvertTo<ezString>();
      
      if (ezReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == ezGetStaticRTTI<ezVariant>())
      {
        ezReflectionUtils::InsertSetPropertyValue(pSet, pObject, *pElemValue);
      }
      else
      {
        // TODO pointer
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }
  else // POD
  {
    if (pProperty->GetCategory() != ezPropertyCategory::Member)
      return;

    ezAbstractMemberProperty* pMember = (ezAbstractMemberProperty*) pProperty;
    ezReflectionUtils::SetMemberPropertyValue(pMember, pObject, *pValue);
  }
}


static void WriteProperties(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType() != nullptr)
  {
    WriteProperties(writer, pRtti->GetParentType(), pObject);
  }

  const auto& props = pRtti->GetProperties();

  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    ezReflectionSerializer::WritePropertyToJSON(writer, props[p], pRtti, pObject);
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

void ezReflectionSerializer::WriteObjectToJSON(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pObject, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode)
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

  const ezVariantArray& va = pVal->Get<ezVariantArray>();

  for (ezUInt32 prop = 0; prop < va.GetCount(); ++prop)
  {
    if (va[prop].GetType() != ezVariant::Type::VariantDictionary)
      continue;

    const ezVariantDictionary& obj = va[prop].Get<ezVariantDictionary>();
    ezReflectionSerializer::ReadPropertyFromJSON(obj, pRtti, pObject);
  }
}

void ezReflectionSerializer::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, const ezRTTI& rtti, void* pObject)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return;

  const ezVariantDictionary& root = reader.GetTopLevelObject();

  ReadJSONObject(root, &rtti, pObject);
}

void* ezReflectionSerializer::ReadObjectFromJSON(ezStreamReaderBase& stream, const ezRTTI*& pRtti, TypeAllocator Allocator)
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