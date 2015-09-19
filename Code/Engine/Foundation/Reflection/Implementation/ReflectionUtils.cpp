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

const ezRTTI* ezReflectionUtils::GetCommonBaseType(const ezRTTI* pRtti1, const ezRTTI* pRtti2)
{
  if (pRtti2 == nullptr)
    return nullptr;

  while (pRtti1 != nullptr)
  {
    const ezRTTI* pRtti2Parent = pRtti2;

    while (pRtti2Parent != nullptr)
    {
      if (pRtti1 == pRtti2Parent)
        return pRtti2Parent;

      pRtti2Parent = pRtti2Parent->GetParentType();
    }

    pRtti1 = pRtti1->GetParentType();
  }

  return nullptr;
}

bool ezReflectionUtils::IsBasicType(const ezRTTI* pRtti)
{
  ezVariant::Type::Enum type = pRtti->GetVariantType();
  return type >= ezVariant::Type::Bool && type <= ezVariant::Type::Uuid;
}

ezVariant ezReflectionUtils::GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject)
{
  if (pProp != nullptr)
  {
    if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
    {
      return static_cast<const ezTypedMemberProperty<const char*>*>(pProp)->GetValue(pObject);
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      void* pValue = nullptr;
      pProp->GetValuePtr(pObject, &pValue);
      return ezVariant(pValue);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || pProp->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
    {
      const ezAbstractEnumerationProperty* pEnumerationProp = static_cast<const ezAbstractEnumerationProperty*>(pProp);
      return pEnumerationProp->GetValue(pObject);
    }
    else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
    {
      return static_cast<const ezTypedMemberProperty<ezVariant>*>(pProp)->GetValue(pObject);
    }
    else
    {
      GetValueFunc func;
      func.m_pProp = pProp;
      func.m_pObject = pObject;

      ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());

      return func.m_Result;
    }
  }

  return ezVariant();
}

void ezReflectionUtils::SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
{
  if (pProp != nullptr && !pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
    {
      static_cast<ezTypedMemberProperty<const char*>*>(pProp)->SetValue(pObject, value.ConvertTo<ezString>().GetData());
      return;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      void* pValue = value.ConvertTo<void*>();
      pProp->SetValuePtr(pObject, &pValue);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || pProp->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
    {
      ezAbstractEnumerationProperty* pEnumerationProp = static_cast<ezAbstractEnumerationProperty*>(pProp);

      // Value can either be an integer or a string (human readable value)
      if (value.IsA<ezString>())
      {
        ezInt64 iValue;
        ezReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<ezString>(), iValue);
        pEnumerationProp->SetValue(pObject, iValue);
      }
      else
      {
        pEnumerationProp->SetValue(pObject, value.Get<ezInt64>());
      }
      return;
    }
    else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
    {
      static_cast<ezTypedMemberProperty<ezVariant>*>(pProp)->SetValue(pObject, value);
    }
    else
    {
      SetValueFunc func;
      func.m_pProp = pProp;
      func.m_pObject = pObject;
      func.m_pValue = &value;

      ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
    }
  }
}

ezVariant ezReflectionUtils::GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex)
{
  if (pProp == nullptr)
  {
    return ezVariant();
  }
  if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
  {
    const char* pData = nullptr;
    pProp->GetValue(pObject, uiIndex, &pData);
    return ezVariant(pData);
  }

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    void* pValue = nullptr;
    pProp->GetValue(pObject, uiIndex, &pValue);
    return ezVariant(pValue);
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
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

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      ezVariant::DispatchTo(func, ezVariantType::VoidPointer);
    else
      ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
    return value;
  }
}

void ezReflectionUtils::SetArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
{
  if (pProp == nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    return;
  }
  if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
  {
    ezString sData = value.ConvertTo<ezString>().GetData();
    const char* pData = sData.GetData();
    pProp->SetValue(pObject, uiIndex, &pData);
    return;
  }
  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    void* pValue = value.ConvertTo<void*>();
    pProp->SetValue(pObject, uiIndex, &pValue);
    return;
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
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

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      ezVariant::DispatchTo(func, ezVariantType::VoidPointer);
    else
      ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
  }
}

void ezReflectionUtils::InsertSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
{
  if (pProp == nullptr || pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    return;
  }

  InsertSetValueFunc func;
  func.m_pProp = pProp;
  func.m_pObject = pObject;
  func.m_pValue = &value;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    ezVariant::DispatchTo(func, ezVariantType::VoidPointer);
  else
    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
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

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    ezVariant::DispatchTo(func, ezVariantType::VoidPointer);
  else
    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
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
    if (prop->GetFlags().IsSet(ezPropertyFlags::StandardType))
      continue;

    switch (prop->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        ezAbstractMemberProperty* memberProp = static_cast<ezAbstractMemberProperty*>(prop);
        const ezRTTI* pPropRtti = memberProp->GetSpecificType();

        if (inout_types.Contains(pPropRtti))
          continue;

        inout_types.Insert(pPropRtti);
        GatherDependentTypes(pPropRtti, inout_types);
      }
      break;
    case ezPropertyCategory::Function:
      break;
    case ezPropertyCategory::Array:
      {
        ezAbstractArrayProperty* pArrayProp = static_cast<ezAbstractArrayProperty*>(prop);
        const ezRTTI* pPropRtti = pArrayProp->GetSpecificType();

        if (inout_types.Contains(pPropRtti))
          continue;

        inout_types.Insert(pPropRtti);
        GatherDependentTypes(pPropRtti, inout_types);
      }
      break;
    case ezPropertyCategory::Set:
      {
        ezAbstractSetProperty* pSetProp = static_cast<ezAbstractSetProperty*>(prop);
        const ezRTTI* pPropRtti = pSetProp->GetSpecificType();

        if (inout_types.Contains(pPropRtti))
          continue;

        inout_types.Insert(pPropRtti);
        GatherDependentTypes(pPropRtti, inout_types);
      }
      break;
    case ezPropertyCategory::Map:
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
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


  while (!dependencies.IsEmpty())
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
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
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
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
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
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        if (ezStringUtils::IsEqual(pProp->GetPropertyName(), szValue))
        {
          ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
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
      for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
      {
        if (pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          if (sValue.IsEqual(pProp->GetPropertyName()))
          {
            ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
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

ezInt64 ezReflectionUtils::DefaultEnumerationValue(const ezRTTI* pEnumerationRtti)
{
  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>() || pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    auto pProp = pEnumerationRtti->GetProperties()[0];
    EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Constant && ezStringUtils::EndsWith(pProp->GetPropertyName(), "::Default"), "First enumeration property must be the default value constant.");
    return static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

ezInt64 ezReflectionUtils::MakeEnumerationValid(const ezRTTI* pEnumerationRtti, ezInt64 iValue)
{
  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>())
  {
    // Find current value
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezInt64 iCurrentValue = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
        if (iCurrentValue == iValue)
          return iValue;
      }
    }

    // Current value not found, return default value
    return ezReflectionUtils::DefaultEnumerationValue(pEnumerationRtti);
  }
  else if (pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    ezInt64 iNewValue = 0;
    // Filter valid bits
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezInt64 iCurrentValue = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
        if ((iCurrentValue & iValue) != 0)
        {
          iNewValue |= iCurrentValue;
        }
      }
    }
    return iNewValue;
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '%s' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_ReflectionUtils);

