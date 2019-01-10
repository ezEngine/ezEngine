#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>

namespace
{
  struct GetTypeFromVariantTypeFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()()
    {
      m_pType = ezGetStaticRTTI<T>();
    }
    const ezRTTI* m_pType;
  };

  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<ezVariantArray>()
  {
    m_pType = nullptr;
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<ezVariantDictionary>()
  {
    m_pType = nullptr;
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<ezReflectedClass*>()
  {
    m_pType = ezGetStaticRTTI<ezReflectedClass>();
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<void*>()
  {
    m_pType = nullptr;
  }

  struct GetTypeFromVariantFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()()
    {
      m_pType = ezGetStaticRTTI<T>();
    }

    const ezVariant* m_pVariant;
    const ezRTTI* m_pType;
  };


  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<ezVariantArray>()
  {
    m_pType = nullptr;
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<ezVariantDictionary>()
  {
    m_pType = nullptr;
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<ezReflectedClass*>()
  {
    m_pType = m_pVariant->Get<ezReflectedClass*>()->GetDynamicRTTI();
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<void*>()
  {
    m_pType = nullptr;
  }

  struct GetValueFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()()
    {
      T value;
      m_pProp->GetValuePtr(m_pObject, &value);
      m_Result = value;
    }

    const ezAbstractMemberProperty* m_pProp;
    const void* m_pObject;
    ezVariant m_Result;
  };

  template <>
  EZ_FORCE_INLINE void GetValueFunc::operator()<ezString>()
  {
    if (m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>())
    {
      ezString value;
      m_pProp->GetValuePtr(m_pObject, &value);
      m_Result = value;
    }
    else if (m_pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
    {
      const char* szTemp = nullptr;
      m_pProp->GetValuePtr(m_pObject, &szTemp);
      m_Result = szTemp;
    }
    else if (m_pProp->GetSpecificType() == ezGetStaticRTTI<ezUntrackedString>())
    {
      ezUntrackedString value;
      m_pProp->GetValuePtr(m_pObject, &value);
      m_Result = value;
    }
  }

  struct SetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->SetValuePtr(m_pObject, &value);
    }

    ezAbstractMemberProperty* m_pProp;
    void* m_pObject;
    const ezVariant* m_pValue;
  };

  template <>
  EZ_FORCE_INLINE void SetValueFunc::operator()<ezString>()
  {
    if (m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>())
    {
      ezString value = m_pValue->Get<ezString>();
      m_pProp->SetValuePtr(m_pObject, &value);
    }
    else if (m_pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
    {
      const ezString& sTemp = m_pValue->Get<ezString>();
      const char* szTemp = sTemp;
      m_pProp->SetValuePtr(m_pObject, &szTemp);
    }
    else if (m_pProp->GetSpecificType() == ezGetStaticRTTI<ezUntrackedString>())
    {
      ezUntrackedString value = m_pValue->Get<ezString>();
      m_pProp->SetValuePtr(m_pObject, &value);
    }
  }

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

  template <>
  EZ_FORCE_INLINE void GetArrayValueFunc::operator()<ezString>()
  {
    EZ_ASSERT_DEBUG(m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>(), "Other string types not implemented");
    ezString value;
    m_pProp->GetValue(m_pObject, m_uiIndex, &value);
    *m_pValue = value;
  }

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

  template <>
  EZ_FORCE_INLINE void SetArrayValueFunc::operator()<ezString>()
  {
    EZ_ASSERT_DEBUG(m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>(), "Other string types not implemented");
    ezString value = m_pValue->ConvertTo<ezString>();
    m_pProp->SetValue(m_pObject, m_uiIndex, &value);
  }

  struct InsertArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->Insert(m_pObject, m_uiIndex, &value);
    }

    ezAbstractArrayProperty* m_pProp;
    void* m_pObject;
    ezUInt32 m_uiIndex;
    const ezVariant* m_pValue;
  };

  template <>
  EZ_FORCE_INLINE void InsertArrayValueFunc::operator()<ezString>()
  {
    EZ_ASSERT_DEBUG(m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>(), "Other string types not implemented");
    ezString value = m_pValue->ConvertTo<ezString>();
    m_pProp->Insert(m_pObject, m_uiIndex, &value);
  }

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


  struct GetMapValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T temp;
      if (m_pProp->GetValue(m_pObject, m_szKey, &temp))
      {
        *m_pValue = temp;
      }
    }

    const ezAbstractMapProperty* m_pProp;
    const void* m_pObject;
    const char* m_szKey;
    ezVariant* m_pValue;
  };

  template <>
  EZ_FORCE_INLINE void GetMapValueFunc::operator()<ezString>()
  {
    ezString temp;
    EZ_ASSERT_DEBUG(m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>(), "Other string types not implemented");
    if (m_pProp->GetValue(m_pObject, m_szKey, &temp))
    {
      *m_pValue = temp;
    }
  }

  struct SetMapValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T value = m_pValue->ConvertTo<T>();
      m_pProp->Insert(m_pObject, m_szKey, &value);
    }

    ezAbstractMapProperty* m_pProp;
    void* m_pObject;
    const char* m_szKey;
    const ezVariant* m_pValue;
  };

  template <>
  EZ_FORCE_INLINE void SetMapValueFunc::operator()<ezString>()
  {
    EZ_ASSERT_DEBUG(m_pProp->GetSpecificType() == ezGetStaticRTTI<ezString>(), "Other string types not implemented");
    ezString value = m_pValue->ConvertTo<ezString>();
    m_pProp->Insert(m_pObject, m_szKey, &value);
  }

  static bool CompareProperties(const void* pObject, const void* pObject2, const ezRTTI* pType)
  {
    if (pType->GetParentType())
    {
      if (!CompareProperties(pObject, pObject2, pType->GetParentType()))
        return false;
    }

    for (auto* pProp : pType->GetProperties())
    {
      if (!ezReflectionUtils::IsEqual(pObject, pObject2, pProp))
        return false;
    }

    return true;
  }

  template <typename T>
  struct SetComponentValueImpl
  {
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 iComponent, double fValue)
    {
      EZ_ASSERT_DEBUG(false, "ezReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct SetComponentValueImpl<ezVec2Template<T>>
  {
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 iComponent, double fValue)
    {
      auto vec = pVector->Get<ezVec2Template<T>>();
      switch (iComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<ezVec3Template<T>>
  {
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 iComponent, double fValue)
    {
      auto vec = pVector->Get<ezVec3Template<T>>();
      switch (iComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<ezVec4Template<T>>
  {
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 iComponent, double fValue)
    {
      auto vec = pVector->Get<ezVec4Template<T>>();
      switch (iComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
        case 3:
          vec.w = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  struct SetComponentValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      SetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    ezVariant* m_pVector;
    ezUInt32 m_iComponent;
    double m_fValue;
  };

  template <typename T>
  struct GetComponentValueImpl
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 iComponent, double& fValue)
    {
      EZ_ASSERT_DEBUG(false, "ezReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec2Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 iComponent, double& fValue)
    {
      const auto& vec = pVector->Get<ezVec2Template<T>>();
      switch (iComponent)
      {
        case 0:
          fValue = static_cast<double>(vec.x);
          break;
        case 1:
          fValue = static_cast<double>(vec.y);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec3Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 iComponent, double& fValue)
    {
      const auto& vec = pVector->Get<ezVec3Template<T>>();
      switch (iComponent)
      {
        case 0:
          fValue = static_cast<double>(vec.x);
          break;
        case 1:
          fValue = static_cast<double>(vec.y);
          break;
        case 2:
          fValue = static_cast<double>(vec.z);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec4Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 iComponent, double& fValue)
    {
      const auto& vec = pVector->Get<ezVec4Template<T>>();
      switch (iComponent)
      {
        case 0:
          fValue = static_cast<double>(vec.x);
          break;
        case 1:
          fValue = static_cast<double>(vec.y);
          break;
        case 2:
          fValue = static_cast<double>(vec.z);
          break;
        case 3:
          fValue = static_cast<double>(vec.w);
          break;
      }
    }
  };

  struct GetComponentValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      GetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    const ezVariant* m_pVector;
    ezUInt32 m_iComponent;
    double m_fValue;
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
  EZ_ASSERT_DEBUG(pRtti != nullptr, "IsBasicType: missing data!");
  ezVariant::Type::Enum type = pRtti->GetVariantType();
  return type >= ezVariant::Type::FirstStandardType && type <= ezVariant::Type::LastStandardType;
}


const ezRTTI* ezReflectionUtils::GetTypeFromVariant(const ezVariant& value)
{
  GetTypeFromVariantFunc func;
  func.m_pVariant = &value;
  func.m_pType = nullptr;
  ezVariant::DispatchTo(func, value.GetType());

  return func.m_pType;
}

const ezRTTI* ezReflectionUtils::GetTypeFromVariant(ezVariantType::Enum type)
{
  GetTypeFromVariantTypeFunc func;
  func.m_pType = nullptr;
  ezVariant::DispatchTo(func, type);

  return func.m_pType;
}

ezUInt32 ezReflectionUtils::GetComponentCount(ezVariantType::Enum type)
{
  switch (type)
  {
    case ezVariant::Type::Vector2:
    case ezVariant::Type::Vector2I:
    case ezVariant::Type::Vector2U:
      return 2;
    case ezVariant::Type::Vector3:
    case ezVariant::Type::Vector3I:
    case ezVariant::Type::Vector3U:
      return 3;
    case ezVariant::Type::Vector4:
    case ezVariant::Type::Vector4I:
    case ezVariant::Type::Vector4U:
      return 4;
    default:
      EZ_REPORT_FAILURE("Not a vector type: '{0}'", type);
      return 0;
  }
}

void ezReflectionUtils::SetComponent(ezVariant& vector, ezUInt32 iComponent, double fValue)
{
  SetComponentValueFunc func;
  func.m_pVector = &vector;
  func.m_iComponent = iComponent;
  func.m_fValue = fValue;
  ezVariant::DispatchTo(func, vector.GetType());
}

double ezReflectionUtils::GetComponent(const ezVariant& vector, ezUInt32 iComponent)
{
  GetComponentValueFunc func;
  func.m_pVector = &vector;
  func.m_iComponent = iComponent;
  ezVariant::DispatchTo(func, vector.GetType());
  return func.m_fValue;
}

ezVariant ezReflectionUtils::GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject)
{
  EZ_ASSERT_DEBUG(pProp != nullptr, "GetMemberPropertyValue: missing data!");

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
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
    ezVariant value;
    pProp->GetValuePtr(pObject, &value);
    return value;
  }
  else
  {
    GetValueFunc func;
    func.m_pProp = pProp;
    func.m_pObject = pObject;

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());

    return func.m_Result;
  }

  return ezVariant();
}

void ezReflectionUtils::SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMemberPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
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
      pEnumerationProp->SetValue(pObject, value.ConvertTo<ezInt64>());
    }
    return;
  }
  else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
  {
    ezVariant tempValue = value;
    pProp->SetValuePtr(pObject, &tempValue);
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

ezVariant ezReflectionUtils::GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    ezLog::Error("GetArrayPropertyValue: Invalid index: {0}", uiIndex);
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

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
    return value;
  }
}

void ezReflectionUtils::SetArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    ezLog::Error("SetArrayPropertyValue: Invalid index: {0}", uiIndex);
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

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
  }
}

void ezReflectionUtils::InsertSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
  {
    ezString sData = value.ConvertTo<ezString>().GetData();
    const char* pData = sData.GetData();
    pProp->Insert(pObject, &pData);
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

void ezReflectionUtils::RemoveSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
  {
    ezString sData = value.ConvertTo<ezString>().GetData();
    const char* pData = sData.GetData();
    pProp->Remove(pObject, &pData);
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

ezVariant ezReflectionUtils::GetMapPropertyValue(const ezAbstractMapProperty* pProp, const void* pObject, const char* szKey)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetMapPropertyValue: missing data!");

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    void* pValue = nullptr;
    pProp->GetValue(pObject, szKey, &pValue);
    return ezVariant(pValue);
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
  {
    ezVariant value;
    if (pProp->GetValue(pObject, szKey, &value))
    {
      return value;
    }
    return ezVariant();
  }
  else
  {
    ezVariant value;
    GetMapValueFunc func;
    func.m_pProp = pProp;
    func.m_szKey = szKey;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
    return value;
  }
}

void ezReflectionUtils::SetMapPropertyValue(ezAbstractMapProperty* pProp, void* pObject, const char* szKey, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMapPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    void* pValue = value.ConvertTo<void*>();
    pProp->Insert(pObject, szKey, &pValue);
    return;
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
  {
    pProp->Insert(pObject, szKey, &value);
    return;
  }
  else
  {
    SetMapValueFunc func;
    func.m_pProp = pProp;
    func.m_szKey = szKey;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
  }
}

void ezReflectionUtils::InsertArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, const ezVariant& value, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex > uiCount)
  {
    ezLog::Error("InsertArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
  {
    ezString sData = value.ConvertTo<ezString>().GetData();
    const char* pData = sData.GetData();
    pProp->Insert(pObject, uiIndex, &pData);
    return;
  }
  if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
  {
    void* pValue = value.ConvertTo<void*>();
    pProp->Insert(pObject, uiIndex, &pValue);
    return;
  }

  if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
  {
    pProp->Insert(pObject, uiIndex, &value);
    return;
  }
  else
  {
    InsertArrayValueFunc func;
    func.m_pProp = pProp;
    func.m_uiIndex = uiIndex;
    func.m_pObject = pObject;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, pProp->GetSpecificType()->GetVariantType());
  }
}

void ezReflectionUtils::RemoveArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    ezLog::Error("RemoveArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  pProp->Remove(pObject, uiIndex);
}

ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  ezHybridArray<ezAbstractProperty*, 32> props;
  pRtti->GetAllProperties(props);
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
    if (prop->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;
    switch (prop->GetCategory())
    {
      case ezPropertyCategory::Member:
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        const ezRTTI* pPropRtti = prop->GetSpecificType();

        if (inout_types.Contains(pPropRtti))
          continue;

        inout_types.Insert(pPropRtti);
        GatherDependentTypes(pPropRtti, inout_types);
      }
      break;
      case ezPropertyCategory::Function:
      case ezPropertyCategory::Constant:
      default:
        break;
    }
  }
}

bool ezReflectionUtils::CreateDependencySortedTypeArray(const ezSet<const ezRTTI*>& types, ezDynamicArray<const ezRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  ezMap<const ezRTTI*, ezSet<const ezRTTI*>> dependencies;

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
      if (accu.ContainsSet(it.Value()))
      {
        out_sortedTypes.PushBack(it.Key());
        accu.Insert(it.Key());
        dependencies.Remove(it);
        bDeadEnd = false;
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
    EZ_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
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
    EZ_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

ezInt64 ezReflectionUtils::DefaultEnumerationValue(const ezRTTI* pEnumerationRtti)
{
  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>() || pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    auto pProp = pEnumerationRtti->GetProperties()[0];
    EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Constant && ezStringUtils::EndsWith(pProp->GetPropertyName(), "::Default"),
                    "First enumeration property must be the default value constant.");
    return static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
  }
  else
  {
    EZ_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
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
    EZ_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

bool ezReflectionUtils::IsEqual(const void* pObject, const void* pObject2, ezAbstractProperty* pProp)
{
  const ezRTTI* pPropType = pProp->GetSpecificType();

  ezVariant vTemp;
  ezVariant vTemp2;
  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pSpecific = static_cast<ezAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        vTemp2 = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();
        void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
        if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
          return false;
        if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
          return true;

        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          return IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
        }
        else
        {
          return pRefrencedObject == pRefrencedObject2;
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType))
        {
          vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          vTemp2 = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
          return vTemp == vTemp2;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);
          void* pSubObject2 = pSpecific->GetPropertyPointer(pObject2);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return IsEqual(pSubObject, pSubObject2, pPropType);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSubObject2 = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pObject, pSubObject);
            pSpecific->GetValuePtr(pObject2, pSubObject2);
            bool bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            pPropType->GetAllocator()->Deallocate(pSubObject);
            pPropType->GetAllocator()->Deallocate(pSubObject2);
            return bEqual;
          }
          else
          {
            // TODO: return false if prop can't be compared?
            return true;
          }
        }
      }
    }
    break;
    case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pSpecific = static_cast<ezAbstractArrayProperty*>(pProp);

      const ezUInt32 uiCount = pSpecific->GetCount(pObject);
      const ezUInt32 uiCount2 = pSpecific->GetCount(pObject2);
      if (uiCount != uiCount2)
        return false;

      if (pSpecific->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          vTemp2 = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
          if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
            return false;
          if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
            continue;

          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            if (!IsEqual(pRefrencedObject, pRefrencedObject2, pPropType))
              return false;
          }
          else
          {
            if (pRefrencedObject != pRefrencedObject2)
              return false;
          }
        }
        return true;
      }
      else
      {
        if (pSpecific->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          for (ezUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            vTemp2 = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
            if (vTemp != vTemp2)
              return false;
          }
          return true;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          void* pSubObject2 = pPropType->GetAllocator()->Allocate<void>();

          bool bEqual = true;
          for (ezUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            pSpecific->GetValue(pObject2, i, pSubObject2);
            bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            if (!bEqual)
              break;
          }

          pPropType->GetAllocator()->Deallocate(pSubObject);
          pPropType->GetAllocator()->Deallocate(pSubObject2);
          return bEqual;
        }
      }
    }
    break;
    case ezPropertyCategory::Set:
    {
      ezAbstractSetProperty* pSpecific = static_cast<ezAbstractSetProperty*>(pProp);

      ezHybridArray<ezVariant, 16> values;
      pSpecific->GetValues(pObject, values);
      ezHybridArray<ezVariant, 16> values2;
      pSpecific->GetValues(pObject2, values2);

      const ezUInt32 uiCount = values.GetCount();
      const ezUInt32 uiCount2 = values2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ||
          (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = values2.Contains(values[i]);
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
        bool bEqual = true;
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            void* pRefrencedObject2 = values2[i].ConvertTo<void*>();
            if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
              return false;
            if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
              continue;

            bEqual = IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
          }
          if (!bEqual)
            break;
        }

        return bEqual;
      }
    }
    break;
    case ezPropertyCategory::Map:
    {
      ezAbstractMapProperty* pSpecific = static_cast<ezAbstractMapProperty*>(pProp);

      ezHybridArray<ezString, 16> keys;
      pSpecific->GetKeys(pObject, keys);
      ezHybridArray<ezString, 16> keys2;
      pSpecific->GetKeys(pObject2, keys2);

      const ezUInt32 uiCount = keys.GetCount();
      const ezUInt32 uiCount2 = keys2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ||
          (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;
          ezVariant value1 = GetMapPropertyValue(pSpecific, pObject, keys[i]);
          ezVariant value2 = GetMapPropertyValue(pSpecific, pObject2, keys[i]);
          bEqual = value1 == value2;
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if ((!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)) &&
               pProp->GetFlags().IsSet(ezPropertyFlags::Class))
      {
        bool bEqual = true;
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;

          if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
          {
            const void* value1 = nullptr;
            const void* value2 = nullptr;
            pSpecific->GetValue(pObject, keys[i], &value1);
            pSpecific->GetValue(pObject2, keys[i], &value2);
            if ((value1 == nullptr) != (value2 == nullptr))
              return false;
            if ((value1 == nullptr) && (value2 == nullptr))
              continue;
            bEqual = IsEqual(value1, value2, pPropType);
          }
          else
          {
            if (pPropType->GetAllocator()->CanAllocate())
            {
              void* value1 = pPropType->GetAllocator()->Allocate<void>();
              EZ_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value1););
              void* value2 = pPropType->GetAllocator()->Allocate<void>();
              EZ_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value2););

              bool bRes1 = pSpecific->GetValue(pObject, keys[i], value1);
              bool bRes2 = pSpecific->GetValue(pObject2, keys[i], value2);
              if (bRes1 != bRes2)
                return false;
              if (!bRes1 && !bRes2)
                continue;
              bEqual = IsEqual(value1, value2, pPropType);
            }
            else
            {
              ezLog::Error("The property '{0}' can not be compared as the type '{1}' cannot be allocated.", pProp->GetPropertyName(),
                           pPropType->GetTypeName());
            }
          }
          if (!bEqual)
            break;
        }
        return bEqual;
      }
    }
    break;
  }
  return true;
}

bool ezReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const ezRTTI* pType)
{
  EZ_ASSERT_DEV(pObject && pObject2 && pType, "invalid type.");
  if (pType->IsDerivedFrom<ezReflectedClass>())
  {
    const ezReflectedClass* pRefObject = static_cast<const ezReflectedClass*>(pObject);
    const ezReflectedClass* pRefObject2 = static_cast<const ezReflectedClass*>(pObject2);
    pType = pRefObject->GetDynamicRTTI();
    if (pType != pRefObject2->GetDynamicRTTI())
      return false;
  }

  return CompareProperties(pObject, pObject2, pType);
}


void ezReflectionUtils::DeleteObject(void* pObject, ezAbstractProperty* pOwnerProperty)
{
  if (!pObject)
    return;

  const ezRTTI* pType = pOwnerProperty->GetSpecificType();
  if (pType->IsDerivedFrom<ezReflectedClass>())
  {
    ezReflectedClass* pRefObject = static_cast<ezReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  if (!pType->GetAllocator()->CanAllocate())
  {
    ezLog::Error("Tried to deallocate object of type '{0}', but it has no allocator.", pType->GetTypeName());
    return;
  }
  pType->GetAllocator()->Deallocate(pObject);
}

ezVariant ezReflectionUtils::GetDefaultVariantFromType(ezVariant::Type::Enum type)
{
  switch (type)
  {
    case ezVariant::Type::Invalid:
      return ezVariant();
    case ezVariant::Type::Bool:
      return ezVariant(false);
    case ezVariant::Type::Int8:
      return ezVariant((ezInt8)0);
    case ezVariant::Type::UInt8:
      return ezVariant((ezUInt8)0);
    case ezVariant::Type::Int16:
      return ezVariant((ezInt16)0);
    case ezVariant::Type::UInt16:
      return ezVariant((ezUInt16)0);
    case ezVariant::Type::Int32:
      return ezVariant((ezInt32)0);
    case ezVariant::Type::UInt32:
      return ezVariant((ezUInt32)0);
    case ezVariant::Type::Int64:
      return ezVariant((ezInt64)0);
    case ezVariant::Type::UInt64:
      return ezVariant((ezUInt64)0);
    case ezVariant::Type::Float:
      return ezVariant(0.0f);
    case ezVariant::Type::Double:
      return ezVariant(0.0);
    case ezVariant::Type::Color:
      return ezVariant(ezColor(1.0f, 1.0f, 1.0f));
    case ezVariant::Type::ColorGamma:
      return ezVariant(ezColorGammaUB(255, 255, 255));
    case ezVariant::Type::Vector2:
      return ezVariant(ezVec2(0.0f, 0.0f));
    case ezVariant::Type::Vector3:
      return ezVariant(ezVec3(0.0f, 0.0f, 0.0f));
    case ezVariant::Type::Vector4:
      return ezVariant(ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
    case ezVariant::Type::Vector2I:
      return ezVariant(ezVec2I32(0, 0));
    case ezVariant::Type::Vector3I:
      return ezVariant(ezVec3I32(0, 0, 0));
    case ezVariant::Type::Vector4I:
      return ezVariant(ezVec4I32(0, 0, 0, 0));
    case ezVariant::Type::Vector2U:
      return ezVariant(ezVec2U32(0, 0));
    case ezVariant::Type::Vector3U:
      return ezVariant(ezVec3U32(0, 0, 0));
    case ezVariant::Type::Vector4U:
      return ezVariant(ezVec4U32(0, 0, 0, 0));
    case ezVariant::Type::Quaternion:
      return ezVariant(ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
    case ezVariant::Type::Matrix3:
      return ezVariant(ezMat3::IdentityMatrix());
    case ezVariant::Type::Matrix4:
      return ezVariant(ezMat4::IdentityMatrix());
    case ezVariant::Type::Transform:
      return ezVariant(ezTransform::IdentityTransform());
    case ezVariant::Type::String:
      return ezVariant("");
    case ezVariant::Type::StringView:
      return ezVariant("");
    case ezVariant::Type::DataBuffer:
      return ezVariant(ezDataBuffer());
    case ezVariant::Type::Time:
      return ezVariant(ezTime());
    case ezVariant::Type::Uuid:
      return ezVariant(ezUuid());
    case ezVariant::Type::Angle:
      return ezVariant(ezAngle());
    case ezVariant::Type::VariantArray:
      return ezVariantArray();
    case ezVariant::Type::VariantDictionary:
      return ezVariantDictionary();
    case ezVariant::Type::ReflectedPointer:
      return ezVariant();
    case ezVariant::Type::VoidPointer:
      return ezVariant();

    default:
      EZ_REPORT_FAILURE("Invalid case statement");
      return ezVariant();
  }
  return ezVariant();
}

ezVariant ezReflectionUtils::GetDefaultValue(const ezAbstractProperty* pProperty)
{
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
  auto type =
      pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
  if (pProperty->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::StandardType))
  {
    if (pAttrib)
    {
      if (pAttrib->GetValue().CanConvertTo(type))
      {
        return pAttrib->GetValue().ConvertTo(type);
      }
    }
    return GetDefaultVariantFromType(type);
  }
  else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
  {
    ezInt64 iValue = ezReflectionUtils::DefaultEnumerationValue(pProperty->GetSpecificType());
    if (pAttrib)
    {
      if (pAttrib->GetValue().CanConvertTo(ezVariantType::Int64))
        iValue = pAttrib->GetValue().ConvertTo<ezInt64>();
    }

    return ezReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue);
  }
  else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::Class))
  {
    return ezUuid();
  }
  EZ_REPORT_FAILURE("Not reachable.");
  return ezVariant();
}

void ezReflectionUtils::SetAllMemberPropertiesToDefault(const ezRTTI* pRtti, void* pObject)
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      const ezVariant defValue = ezReflectionUtils::GetDefaultValue(pProp);

      ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pProp), pObject, defValue);
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_ReflectionUtils);
