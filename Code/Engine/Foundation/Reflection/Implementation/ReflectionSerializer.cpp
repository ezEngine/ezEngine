#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionSerializer.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>

ezReflectionSerializer::ezReflectionSerializer(ezReflectionAdapter* pAdapter) : m_pAdapter(pAdapter)
{
  m_pContext = pAdapter->GetContext();
  m_bSerializeReadOnly = false;
}




void ezReflectionSerializer::WritePropertyToJSON(ezJSONWriter& writer, const ezReflectedPropertyWrapper& prop, const ezReflectedObjectWrapper& object)
{
  if (prop.m_Flags.IsSet(ezPropertyFlags::ReadOnly) && !m_bSerializeReadOnly)
    return;

  ezReflectedTypeWrapper propType = m_pAdapter->GetTypeInfo(prop.m_pType);

  switch (prop.m_Category)
  {
  case ezPropertyCategory::Member:
    {
      if (propType.m_Flags.IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
      {
        writer.BeginObject();
        writer.AddVariableString("t", propType.m_Flags.IsSet(ezTypeFlags::IsEnum) ? "ezEnum" : "ezBitflags");
        writer.AddVariableString("n", prop.m_szName);
        writer.AddVariableInt64("v", m_pAdapter->GetPropertyValue(object, prop, ezVariant()).ConvertTo<ezInt64>());
        writer.EndObject();
      }
      else if (m_pAdapter->IsStandardType(prop.m_pType))
      {
        ezVariant value = m_pAdapter->GetPropertyValue(object, prop, ezVariant());
        writer.BeginObject();
        writer.AddVariableString("t", propType.m_szName);
        writer.AddVariableString("n", prop.m_szName);
        writer.AddVariableVariant("v", value);
        writer.EndObject();
      }
      else if (prop.m_Flags.IsSet(ezPropertyFlags::Pointer))
      {
        void* pValue = m_pAdapter->GetPropertyValue(object, prop, ezVariant()).ConvertTo<void*>();
        ezUuid uObject = prop.m_Flags.IsSet(ezPropertyFlags::PointerOwner) ? m_pContext->EnqueObject(pValue, prop.m_pType) : m_pContext->GetObjectGUID(pValue);
        writer.BeginObject();
        writer.AddVariableString("t", propType.m_szName);
        writer.AddVariableString("n", prop.m_szName);
        writer.AddVariableVariant("v", uObject);
        writer.EndObject();
      }
      else if (m_pAdapter->GetPropertyCount(prop.m_pType) > 0)
      {
        // Do we have direct access to the property?
        if (m_pAdapter->CanGetDirectPropertyPointer(object, prop, ezVariant()))
        {
          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          writer.AddVariableString("n", prop.m_szName);

          const ezReflectedObjectWrapper propObject = m_pAdapter->GetDirectPropertyPointer(object, prop, ezVariant());
          WriteJSONObject(writer, propObject, "v");

          writer.EndObject();
        }
        // If the property is behind an accessor, we need to retrieve it first.
        else if (m_pAdapter->CanCreateObject(prop.m_pType))
        {
          ezReflectedObjectWrapper propObject = m_pAdapter->CreateObject(prop.m_pType);
          m_pAdapter->GetPropertyObject(object, prop, ezVariant(), propObject);

          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          writer.AddVariableString("n", prop.m_szName);

          WriteJSONObject(writer, propObject, "v");

          writer.EndObject();
          m_pAdapter->DeleteObject(propObject);
        }
      }
    }
    break;
  case ezPropertyCategory::Array:
    {
      writer.BeginObject();
      writer.AddVariableString("t", "$array");
      writer.AddVariableString("n", prop.m_szName);
      writer.BeginArray("v");

      ezUInt32 uiCount = m_pAdapter->GetArrayElementCount(object, prop);
      if (m_pAdapter->IsStandardType(prop.m_pType))
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          writer.BeginObject();
          writer.AddVariableString("t", propType.m_szName);
          ezVariant value = m_pAdapter->GetPropertyValue(object, prop, i);
          writer.AddVariableVariant("v", value);
          writer.EndObject();
        }
      }
      else if (prop.m_Flags.IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          void* pValue = m_pAdapter->GetPropertyValue(object, prop, i).ConvertTo<void*>();
          ezUuid uObject = prop.m_Flags.IsSet(ezPropertyFlags::PointerOwner) ? m_pContext->EnqueObject(pValue, prop.m_pType) : m_pContext->GetObjectGUID(pValue);
          writer.BeginObject();
          writer.AddVariableString("t", propType.m_szName);
          writer.AddVariableVariant("v", uObject);
          writer.EndObject();
        }
      }
      else if (m_pAdapter->CanCreateObject(prop.m_pType))
      {
        ezReflectedObjectWrapper propObject = m_pAdapter->CreateObject(prop.m_pType);

        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          m_pAdapter->GetPropertyObject(object, prop, i, propObject);
          writer.BeginObject();
          writer.AddVariableString("t", "$s"); // struct property
          WriteJSONObject(writer, propObject, "v");
          writer.EndObject();
        }
        m_pAdapter->DeleteObject(propObject);
      }

      writer.EndArray();
      writer.EndObject();
    }
    break;
  case ezPropertyCategory::Set:
    {
      writer.BeginObject();
      writer.AddVariableString("t", "$set");
      writer.AddVariableString("n", prop.m_szName);
      writer.BeginArray("v");

      ezHybridArray<ezVariant, 16> keys;
      m_pAdapter->GetSetContent(object, prop, keys);

      if (m_pAdapter->IsStandardType(prop.m_pType))
      {
        for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          writer.BeginObject();
          writer.AddVariableString("t", propType.m_szName);
          writer.AddVariableVariant("v", keys[i]);
          writer.EndObject();
        }
      }
      else if (prop.m_Flags.IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          void* pValue = keys[i].ConvertTo<void*>();
          ezUuid uObject = prop.m_Flags.IsSet(ezPropertyFlags::PointerOwner) ? m_pContext->EnqueObject(pValue, prop.m_pType) : m_pContext->GetObjectGUID(pValue);
          writer.BeginObject();
          writer.AddVariableString("t", propType.m_szName);
          writer.AddVariableVariant("v", uObject);
          writer.EndObject();
        }
      }

      writer.EndArray();
      writer.EndObject();
    }
    break;
  default:
    break;
  }
}


void ezReflectionSerializer::ReadPropertyFromJSON(const ezVariantDictionary& propDict, ezReflectedObjectWrapper& object)
{
  ezVariant* pName;
  if (!propDict.TryGetValue("n", pName))
    return;

  ezVariant* pType;
  if (!propDict.TryGetValue("t", pType))
    return;

  ezVariant* pValue;
  if (!propDict.TryGetValue("v", pValue))
    return;

  void* pProp = m_pAdapter->FindPropertyByName(object.m_pType, pName->ConvertTo<ezString>().GetData());
  if (pProp == nullptr)
    return;

  ezReflectedPropertyWrapper prop = m_pAdapter->GetPropertyInfo(pProp);
  ezReflectedTypeWrapper propType = m_pAdapter->GetTypeInfo(prop.m_pType);

  if (prop.m_Flags.IsSet(ezPropertyFlags::ReadOnly))
    return;


  const ezRTTI* pRtti = (const ezRTTI*)object.m_pType;
  void* pObject = object.m_pObject;

  ezAbstractProperty* pProperty = pRtti->FindPropertyByName(pName->ConvertTo<ezString>().GetData());

  if (pProperty == nullptr)
    return;



  const ezString sType = pType->ConvertTo<ezString>();

  if (prop.m_Category == ezPropertyCategory::Member)
  {
    if (m_pAdapter->IsStandardType(prop.m_pType) || propType.m_Flags.IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
    {
      m_pAdapter->SetPropertyValue(object, prop, ezVariant(), *pValue);
    }
    else if (prop.m_Flags.IsSet(ezPropertyFlags::Pointer) && pValue->IsA<ezUuid>())
    {
      ezUuid uObject = pValue->Get<ezUuid>();
      void* pPtrValue = m_pContext->GetObjectByGUID(uObject);
      m_pAdapter->SetPropertyValue(object, prop, ezVariant(), pPtrValue);
    }
    else if (sType == "$s")
    {
      if (!pValue->IsA<ezVariantDictionary>())
        return;

      if (m_pAdapter->CanGetDirectPropertyPointer(object, prop, ezVariant()))
      {
        ezReflectedObjectWrapper propObject = m_pAdapter->GetDirectPropertyPointer(object, prop, ezVariant());
        ReadJSONObject(pValue->Get<ezVariantDictionary>(), propObject);
      }
      else if (m_pAdapter->CanCreateObject(prop.m_pType))
      {
        ezReflectedObjectWrapper propObject = m_pAdapter->CreateObject(prop.m_pType);
        ReadJSONObject(pValue->Get<ezVariantDictionary>(), propObject);
        m_pAdapter->SetPropertyObject(object, prop, ezVariant(), propObject);
        m_pAdapter->DeleteObject(propObject);
      }
    }
  }
  else if (sType == "$array") // array property
  {
    if (prop.m_Category != ezPropertyCategory::Array)
      return;

    if (!pValue->IsA<ezVariantArray>())
      return;

    const ezVariantArray& data = pValue->Get<ezVariantArray>();

    m_pAdapter->SetArrayElementCount(object, prop, data.GetCount());

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

        ezReflectedObjectWrapper propObject = m_pAdapter->CreateObject(prop.m_pType);
        ReadJSONObject(pElemValue->Get<ezVariantDictionary>(), propObject);
        m_pAdapter->SetPropertyObject(object, prop, i, propObject);
        m_pAdapter->DeleteObject(propObject);
      }
      else if (prop.m_Flags.IsSet(ezPropertyFlags::Pointer))
      {
        ezUuid uObject = pElemValue->Get<ezUuid>();
        void* pValue = m_pContext->GetObjectByGUID(uObject);
        m_pAdapter->SetPropertyValue(object, prop, i, pValue);
      }
      else // pod element
      {
        m_pAdapter->SetPropertyValue(object, prop, i, *pElemValue);
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

}


void ezReflectionSerializer::WriteProperties(ezJSONWriter& writer, const ezReflectedObjectWrapper& object)
{
  if (m_pAdapter->GetParentType(object.m_pType) != nullptr)
  {
    ezReflectedObjectWrapper parentObject(m_pAdapter->GetParentType(object.m_pType), object.m_pObject);
    WriteProperties(writer, parentObject);
  }

  const ezUInt32 uiPropertyCount = m_pAdapter->GetPropertyCount(object.m_pType);

  for (ezUInt32 p = 0; p < uiPropertyCount; ++p)
  {
    void* pProp = m_pAdapter->GetProperty(object.m_pType, p);
    const ezReflectedPropertyWrapper prop = m_pAdapter->GetPropertyInfo(pProp);
    ezReflectionSerializer::WritePropertyToJSON(writer, prop, object);
  }
}


void ezReflectionSerializer::WriteJSONObject(ezJSONWriter& writer, const ezReflectedObjectWrapper& object, const char* szObjectName)
{
  ezReflectedTypeWrapper type = m_pAdapter->GetTypeInfo(object.m_pType);
  writer.BeginObject(szObjectName);

  if (object.m_pType != nullptr && object.m_pObject != nullptr)
  {
    writer.AddVariableString("t", type.m_szName);

    writer.BeginArray("p");

    WriteProperties(writer, object);

    writer.EndArray();
  }
  else
  {
    writer.AddVariableString("t", "null");
  }

  writer.EndObject();
}


void ezReflectionSerializer::ReadJSONObject(const ezVariantDictionary& root, ezReflectedObjectWrapper& object)
{
  ezVariant* pVal;

  if (object.m_pObject == nullptr || !root.TryGetValue("p", pVal) || !pVal->IsA<ezVariantArray>())
    return;

  const ezVariantArray& va = pVal->Get<ezVariantArray>();

  for (ezUInt32 prop = 0; prop < va.GetCount(); ++prop)
  {
    if (va[prop].GetType() != ezVariant::Type::VariantDictionary)
      continue;

    const ezVariantDictionary& obj = va[prop].Get<ezVariantDictionary>();
    ezReflectionSerializer::ReadPropertyFromJSON(obj, object);
  }
}




////////////////////////////////////////////////////////////////////////
// ezReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void ezReflectionSerializer::WriteObjectToJSON(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pObject, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode)
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(WhitespaceMode);

  ezRttiSerializationContext context;
  ezRttiAdapter adapter(&context);
  ezReflectionSerializer serializer(&adapter);
  serializer.WriteJSONObject(writer, ezReflectedObjectWrapper(pRtti, const_cast<void*>(pObject)), nullptr);
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

  ezRttiSerializationContext context;
  ezRttiAdapter adapter(&context);
  ezReflectionSerializer serializer(&adapter);
  serializer.ReadJSONObject(root, ezReflectedObjectWrapper(pRtti, pObject));

  return pObject;
}

void ezReflectionSerializer::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, const ezRTTI& rtti, void* pObject)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return;

  const ezVariantDictionary& root = reader.GetTopLevelObject();

  ezRttiSerializationContext context;
  ezRttiAdapter adapter(&context);
  ezReflectionSerializer serializer(&adapter);
  serializer.ReadJSONObject(root, ezReflectedObjectWrapper(&rtti, pObject));
}