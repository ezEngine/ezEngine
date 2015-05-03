#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>

namespace
{
  struct GetConstantValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      m_Result = static_cast<const ezTypedConstantProperty<T>*>(m_pProp)->GetValue();
    }

    const ezAbstractConstantProperty* m_pProp;
    ezVariant m_Result;
  };

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

  struct GetArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value;
      m_pProp->GetValue(m_pObject, m_uiIndex, &value);
      *m_pValue = value;
    }

    const ezAbstractArrayProperty* m_pProp;
    const void* m_pObject;
    ezUInt32 m_uiIndex;
    ezVariant* m_pValue;
  };

  struct SetArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->SetValue(m_pObject, m_uiIndex, &value);
    }

    ezAbstractArrayProperty* m_pProp;
    void* m_pObject;
    ezUInt32 m_uiIndex;
    const ezVariant* m_pValue;
  };

  struct InsertSetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->Insert(m_pObject, &value);
    }

    ezAbstractSetProperty* m_pProp;
    void* m_pObject;
    const ezVariant* m_pValue;
  };

  struct RemoveSetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->Remove(m_pObject, &value);
    }

    ezAbstractSetProperty* m_pProp;
    void* m_pObject;
    const ezVariant* m_pValue;
  };
}

bool ezReflectionUtils::IsBasicType(const ezRTTI* pRtti)
{
  ezVariant::Type::Enum type = pRtti->GetVariantType();
  return type >= ezVariant::Type::Bool && type <= ezVariant::Type::Uuid;
}

ezVariant ezReflectionUtils::GetConstantPropertyValue(const ezAbstractConstantProperty* pProp)
{
  if (pProp != nullptr)
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
      return static_cast<const ezTypedConstantProperty<const char*>*>(pProp)->GetValue();

    GetConstantValueFunc func;
    func.m_pProp = pProp;

    ezVariant::DispatchTo(func, pProp->GetPropertyType()->GetVariantType());

    return func.m_Result;
  }

  return ezVariant();
}

ezVariant ezReflectionUtils::GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject)
{
  if (pProp != nullptr)
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
    {
      return static_cast<const ezTypedMemberProperty<const char*>*>(pProp)->GetValue(pObject);
    }
    else if (pProp->GetPropertyType()->IsDerivedFrom<ezEnumBase>() || pProp->GetPropertyType()->IsDerivedFrom<ezBitflagsBase>())
    {
      const ezAbstractEnumerationProperty* pEnumerationProp = static_cast<const ezAbstractEnumerationProperty*>(pProp);
      return pEnumerationProp->GetValue(pObject);
    }
    else if (pProp->GetPropertyType() == ezGetStaticRTTI<ezVariant>())
    {
      return static_cast<const ezTypedMemberProperty<ezVariant>*>(pProp)->GetValue(pObject);
    }
    else
    {
      GetValueFunc func;
      func.m_pProp = pProp;
      func.m_pObject = pObject;

      ezVariant::DispatchTo(func, pProp->GetPropertyType()->GetVariantType());

      return func.m_Result;
    }
  }

  return ezVariant();
}

void ezReflectionUtils::SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
{
  if (pProp != nullptr && !pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    if (pProp->GetPropertyType() == ezGetStaticRTTI<const char*>())
    {
      static_cast<ezTypedMemberProperty<const char*>*>(pProp)->SetValue(pObject, value.ConvertTo<ezString>().GetData());
      return;
    }
    else if (pProp->GetPropertyType()->IsDerivedFrom<ezEnumBase>() || pProp->GetPropertyType()->IsDerivedFrom<ezBitflagsBase>())
    {
      ezAbstractEnumerationProperty* pEnumerationProp = static_cast<ezAbstractEnumerationProperty*>(pProp);

      // Value can either be an integer or a string (human readable value)
      if (value.IsA<ezString>())
      {
        ezInt64 iValue;
        ezReflectionUtils::StringToEnumeration(pProp->GetPropertyType(), value.Get<ezString>(), iValue);
        pEnumerationProp->SetValue(pObject, iValue);
      }
      else
      {
        pEnumerationProp->SetValue(pObject, value.Get<ezInt64>());
      }
      return;
    }
    else if (pProp->GetPropertyType() == ezGetStaticRTTI<ezVariant>())
    {
      static_cast<ezTypedMemberProperty<ezVariant>*>(pProp)->SetValue(pObject, value);
    }
    else
    {
      SetValueFunc func;
      func.m_pProp = pProp;
      func.m_pObject = pObject;
      func.m_pValue = &value;

      ezVariant::DispatchTo(func, pProp->GetPropertyType()->GetVariantType());
    }
  }
}

ezVariant ezReflectionUtils::GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex)
{
  if (pProp == nullptr)
  {
    return ezVariant();
  }
  if (pProp->GetElementType() == ezGetStaticRTTI<const char*>())
  {
    const char* pData = nullptr;
    pProp->GetValue(pObject, uiIndex, &pData);
    return ezVariant(pData);
  }

  if (pProp->GetElementType() == ezGetStaticRTTI<ezVariant>())
  {
    ezVariant value;
    pProp->GetValue(pObject, uiIndex, &value);
    return value;
  }
  else
  {
    ezVariant value;
    GetArrayValueFunc func;
    func.m_pProp = pProp;
    func.m_uiIndex = uiIndex;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetElementType()->GetVariantType());
    return value;
  }
}

void ezReflectionUtils::SetArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
{
  if (pProp == nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    return;
  }
  if (pProp->GetElementType() == ezGetStaticRTTI<const char*>())
  {
    const char* pData = value.ConvertTo<ezString>().GetData();
    pProp->SetValue(pObject, uiIndex, &pData);
    return;
  }

  if (pProp->GetElementType() == ezGetStaticRTTI<ezVariant>())
  {
    pProp->SetValue(pObject, uiIndex, &value);
    return;
  }
  else
  {
    SetArrayValueFunc func;
    func.m_pProp = pProp;
    func.m_uiIndex = uiIndex;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetElementType()->GetVariantType());
  }
}

void ezReflectionUtils::InsertSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, ezVariant& value)
{
  if (pProp == nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    return;
  }

  InsertSetValueFunc func;
  func.m_pProp = pProp;
  func.m_pObject = pObject;
  func.m_pValue = &value;

  ezVariant::DispatchTo(func, pProp->GetElementType()->GetVariantType());
}

void ezReflectionUtils::RemoveSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, ezVariant& value)
{
  if (pProp == nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    return;
  }

  RemoveSetValueFunc func;
  func.m_pProp = pProp;
  func.m_pObject = pObject;
  func.m_pValue = &value;

  ezVariant::DispatchTo(func, pProp->GetElementType()->GetVariantType());
}

ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  const ezArrayPtr<ezAbstractProperty*>& props = pRtti->GetProperties();
  if (uiPropertyIndex < props.GetCount())
  {
    ezAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == ezPropertyCategory::Member)
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
    if (pProp->GetCategory() == ezPropertyCategory::Member)
      return static_cast<ezAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

static void WriteJSONObject(ezJSONWriter& writer, const ezRTTI* pRtti, const void* pObject, const char* szObjectName);

void ezReflectionUtils::WritePropertyToJSON(ezJSONWriter& writer, const ezAbstractProperty* pProp, const ezRTTI* pRtti, const void* pObject)
{
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      const ezAbstractMemberProperty* prop = static_cast<const ezAbstractMemberProperty*>(pProp);
      const ezRTTI* pPropRtti = prop->GetPropertyType();

      if (prop->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
        return;

      if (pPropRtti->IsDerivedFrom<ezEnumBase>() || pPropRtti->IsDerivedFrom<ezBitflagsBase>())
      {
        const ezAbstractEnumerationProperty* pEnumProp = static_cast<const ezAbstractEnumerationProperty*>(prop);
        writer.BeginObject();
        writer.AddVariableString("t", prop->GetPropertyType()->IsDerivedFrom<ezEnumBase>() ? "ezEnum" : "ezBitflags");
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

void ezReflectionUtils::ReadPropertyFromJSON(const ezVariantDictionary& prop, const ezRTTI* pRtti, void* pObject)
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
    ezReflectionUtils::WritePropertyToJSON(writer, props[p], pRtti, pObject);
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

  const ezVariantArray& va = pVal->Get<ezVariantArray>();

  for (ezUInt32 prop = 0; prop < va.GetCount(); ++prop)
  {
    if (va[prop].GetType() != ezVariant::Type::VariantDictionary)
      continue;

    const ezVariantDictionary& obj = va[prop].Get<ezVariantDictionary>();
    ezReflectionUtils::ReadPropertyFromJSON(obj, pRtti, pObject);
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


void ezReflectionUtils::GatherTypesDerivedFromClass(const ezRTTI* pRtti, ezSet<const ezRTTI*>& out_types, bool bIncludeDependencies)
{
  out_types.Clear();

  ezRTTI* pFirst = ezRTTI::GetFirstInstance();
  while (pFirst != nullptr)
  {
    if (pFirst->IsDerivedFrom(pRtti))
    {
      out_types.Insert(pFirst);
      if (bIncludeDependencies)
      {
        GatherDependentTypes(pFirst, out_types);
      }
    }
    pFirst = pFirst->GetNextInstance();
  }
}

void ezReflectionUtils::GatherDependentTypes(const ezRTTI* pRtti, ezSet<const ezRTTI*>& inout_types)
{
  const ezRTTI* pParentRtti = pRtti->GetParentType();
  if (pParentRtti != nullptr)
  {
    inout_types.Insert(pParentRtti);
    GatherDependentTypes(pParentRtti, inout_types);
  }

  const ezArrayPtr<ezAbstractProperty*>& rttiProps = pRtti->GetProperties();
  const ezUInt32 uiCount = rttiProps.GetCount();

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        ezAbstractMemberProperty* memberProp = static_cast<ezAbstractMemberProperty*>(prop);
        const ezRTTI* pMemberPropRtti = memberProp->GetPropertyType();
        ezVariant::Type::Enum memberType = pMemberPropRtti->GetVariantType();

        if (memberType == ezVariant::Type::Invalid || memberProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          if (pMemberPropRtti != nullptr)
          {
            // static rtti or dynamic rtti classes
            inout_types.Insert(pMemberPropRtti);
            GatherDependentTypes(pMemberPropRtti, inout_types);
          }
        }
        else if (memberProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          // Ignore PODs
        }
        else
        {
          EZ_ASSERT_DEV(false, "Member property found that is not understood: Property '%s' of type '%s'!",
            memberProp->GetPropertyName(), pMemberPropRtti->GetTypeName());
        }
      }
      break;
    case ezPropertyCategory::Function:
      break;
    case ezPropertyCategory::Array:
      EZ_ASSERT_DEV(false, "Arrays are not supported yet!");
      break;
    }
  }
}

bool ezReflectionUtils::CreateDependencySortedTypeArray(const ezSet<const ezRTTI*>& types, ezDynamicArray<const ezRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  ezMap<const ezRTTI*, ezSet<const ezRTTI*> > dependencies;

  ezSet<const ezRTTI*> accu;

  for (const ezRTTI* pType : types)
  {
    auto it = dependencies.Insert(pType, ezSet<const ezRTTI*>());
    GatherDependentTypes(pType, it.Value());
  }


  while(!dependencies.IsEmpty())
  {
    bool bDeadEnd = true;
    for (auto it = dependencies.GetIterator(); it.IsValid(); ++it)
    {
      // Are the types dependencies met?
      if (accu.Contains(it.Value()))
      {
        out_sortedTypes.PushBack(it.Key());
        bDeadEnd = false;
        dependencies.Remove(it);
        accu.Insert(it.Key());
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  return true;
}

bool ezReflectionUtils::EnumerationToString(const ezRTTI* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput)
{
  out_sOutput.Clear();
  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties())
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezVariant value = ezReflectionUtils::GetConstantPropertyValue(static_cast<const ezAbstractConstantProperty*>(pProp));
        if (value.ConvertTo<ezInt64>() == iValue)
        {
          out_sOutput = pProp->GetPropertyName();
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties())
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezVariant value = ezReflectionUtils::GetConstantPropertyValue(static_cast<const ezAbstractConstantProperty*>(pProp));
        if ((value.ConvertTo<ezInt64>() & iValue) != 0)
        {
          out_sOutput.Append(pProp->GetPropertyName(), "|");
        }
      }
    }
    out_sOutput.Shrink(0, 1);
    return true;
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

bool ezReflectionUtils::StringToEnumeration(const ezRTTI* pEnumerationRtti, const char* szValue, ezInt64& out_iValue)
{
  out_iValue = 0;
  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties())
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        if (ezStringUtils::IsEqual(pProp->GetPropertyName(), szValue))
        {
          ezVariant value = ezReflectionUtils::GetConstantPropertyValue(static_cast<const ezAbstractConstantProperty*>(pProp));
          out_iValue = value.ConvertTo<ezInt64>();
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    ezStringBuilder temp = szValue;
    ezHybridArray<ezStringView, 32> values;
    temp.Split(false, values, "|");
    for (auto sValue : values)
    {
      for (auto pProp : pEnumerationRtti->GetProperties())
      {
        if (pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          if (sValue.IsEqual(pProp->GetPropertyName()))
          {
            ezVariant value = ezReflectionUtils::GetConstantPropertyValue(static_cast<const ezAbstractConstantProperty*>(pProp));
            out_iValue |= value.ConvertTo<ezInt64>();
          }
        }
      }
    }
    return true;
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_ReflectionUtils);

