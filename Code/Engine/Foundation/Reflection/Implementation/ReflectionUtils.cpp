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
      return static_cast<const ezTypedMemberProperty<const char*>*>(pProp)->GetValue(pObject);

    if (pProp->GetPropertyType()->IsDerivedFrom<ezEnumBase>() || pProp->GetPropertyType()->IsDerivedFrom<ezBitflagsBase>())
    {
      const ezAbstractEnumerationProperty* pEnumerationProp = static_cast<const ezAbstractEnumerationProperty*>(pProp);
      return pEnumerationProp->GetValue(pObject);
    }

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

    if (pProp->GetPropertyType()->IsDerivedFrom<ezEnumBase>() || pProp->GetPropertyType()->IsDerivedFrom<ezBitflagsBase>())
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

    if (pProp->GetPropertyType() == ezGetStaticRTTI<ezVariant>())
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
      else IF_HANDLE_TYPE(ezUuid,  AddVariableUuid)
      else IF_HANDLE_TYPE(ezConstCharPtr,  AddVariableString)
      else IF_HANDLE_TYPE(ezVariant,  AddVariableVariant)
      else IF_HANDLE_TYPE(ezString,  AddVariableString)

      else if (prop->GetPropertyType()->IsDerivedFrom<ezEnumBase>() || prop->GetPropertyType()->IsDerivedFrom<ezBitflagsBase>())
      {
        const ezAbstractEnumerationProperty* pEnumProp = static_cast<const ezAbstractEnumerationProperty*>(prop);
        writer.BeginObject();
        writer.AddVariableString("t", prop->GetPropertyType()->IsDerivedFrom<ezEnumBase>() ? "ezEnum" : "ezBitflags");
        writer.AddVariableString("n", prop->GetPropertyName());
        writer.AddVariableInt64("v", pEnumProp->GetValue(pObject));
        writer.EndObject();
      }
      else if (prop->GetPropertyType()->GetProperties().GetCount() > 0 && prop->GetPropertyPointer(pObject) != nullptr)
      {
        writer.BeginObject();
        writer.AddVariableString("t", "$s"); // struct property
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

    if (sType != "$s") // not a struct property
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
    case ezAbstractProperty::Member:
      {
        ezAbstractMemberProperty* memberProp = static_cast<ezAbstractMemberProperty*>(prop);
        const ezRTTI* pMemberPropRtti = memberProp->GetPropertyType();
        ezVariant::Type::Enum memberType = pMemberPropRtti->GetVariantType();
       
        if (memberType == ezVariant::Type::Invalid || memberType == ezVariant::Type::ReflectedPointer)
        {
          if (pMemberPropRtti != nullptr)
          {
            // static rtti or dynamic rtti classes
            inout_types.Insert(pMemberPropRtti);
            GatherDependentTypes(pMemberPropRtti, inout_types);
          }
        }
        else if (memberType >= ezVariant::Type::Bool && memberType <= ezVariant::Type::Uuid)
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
    case ezAbstractProperty::Function:
      break;
    case ezAbstractProperty::Array:
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
      if (pProp->GetCategory() == ezAbstractProperty::Constant)
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
      if (pProp->GetCategory() == ezAbstractProperty::Constant)
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
      if (pProp->GetCategory() == ezAbstractProperty::Constant)
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
        if (pProp->GetCategory() == ezAbstractProperty::Constant)
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

