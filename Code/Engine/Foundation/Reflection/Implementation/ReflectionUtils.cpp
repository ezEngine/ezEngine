#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

namespace
{
  // for some reason MSVC does not accept the template keyword here
#if EZ_ENABLED(EZ_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) functor.template operator()<type>(std::forward<Args>(args)...)
#endif

  template <typename Functor, class... Args>
  void DispatchTo(Functor& ref_functor, const ezAbstractProperty* pProp, Args&&... args)
  {
    const bool bIsPtr = pProp->GetFlags().IsSet(ezPropertyFlags::Pointer);
    if (bIsPtr)
    {
      CALL_FUNCTOR(ref_functor, ezTypedPointer);
      return;
    }
    else if (pProp->GetSpecificType() == ezGetStaticRTTI<const char*>())
    {
      CALL_FUNCTOR(ref_functor, const char*);
      return;
    }
    else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezUntrackedString>())
    {
      CALL_FUNCTOR(ref_functor, ezUntrackedString);
      return;
    }
    else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
    {
      CALL_FUNCTOR(ref_functor, ezVariant);
      return;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
    {
      ezVariant::DispatchTo(ref_functor, pProp->GetSpecificType()->GetVariantType(), std::forward<Args>(args)...);
      return;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::IsEnum))
    {
      CALL_FUNCTOR(ref_functor, ezEnumBase);
      return;
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Bitflags))
    {
      CALL_FUNCTOR(ref_functor, ezBitflagsBase);
      return;
    }
    else if (pProp->GetSpecificType()->GetVariantType() == ezVariantType::TypedObject)
    {
      CALL_FUNCTOR(ref_functor, ezTypedObject);
      return;
    }

    EZ_REPORT_FAILURE("Unknown dispatch type");
  }

#undef CALL_FUNCTOR

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
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<ezTypedPointer>()
  {
    m_pType = nullptr;
  }
  template <>
  EZ_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<ezTypedObject>()
  {
    m_pType = nullptr;
  }

  //////////////////////////////////////////////////////////////////////////



  template <typename T>
  struct ezPropertyValue
  {
    using Type = T;
    using StorageType = typename ezVariantTypeDeduction<T>::StorageType;
  };
  template <>
  struct ezPropertyValue<ezEnumBase>
  {
    using Type = ezInt64;
    using StorageType = ezInt64;
  };
  template <>
  struct ezPropertyValue<ezBitflagsBase>
  {
    using Type = ezInt64;
    using StorageType = ezInt64;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct ezVariantFromProperty
  {
    ezVariantFromProperty(ezVariant& value, const ezAbstractProperty* pProp)
      : m_value(value)
    {
      EZ_IGNORE_UNUSED(pProp);
    }
    ~ezVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = m_tempValue;
    }

    operator void*()
    {
      return &m_tempValue;
    }

    ezVariant& m_value;
    typename ezPropertyValue<T>::Type m_tempValue = {};
    bool m_bSuccess = true;
  };

  template <>
  struct ezVariantFromProperty<ezVariant>
  {
    ezVariantFromProperty(ezVariant& value, const ezAbstractProperty* pProp)
      : m_value(value)
    {
      EZ_IGNORE_UNUSED(pProp);
    }

    operator void*()
    {
      return &m_value;
    }

    ezVariant& m_value;
    bool m_bSuccess = true;
  };

  template <>
  struct ezVariantFromProperty<ezTypedPointer>
  {
    ezVariantFromProperty(ezVariant& value, const ezAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
    }
    ~ezVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = ezVariant(m_ptr, m_pProp->GetSpecificType());
    }

    operator void*()
    {
      return &m_ptr;
    }

    ezVariant& m_value;
    const ezAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  template <>
  struct ezVariantFromProperty<ezTypedObject>
  {
    ezVariantFromProperty(ezVariant& value, const ezAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
      m_ptr = m_pProp->GetSpecificType()->GetAllocator()->Allocate<void>();
    }
    ~ezVariantFromProperty()
    {
      if (m_bSuccess)
        m_value.MoveTypedObject(m_ptr, m_pProp->GetSpecificType());
      else
        m_pProp->GetSpecificType()->GetAllocator()->Deallocate(m_ptr);
    }

    operator void*()
    {
      return m_ptr;
    }

    ezVariant& m_value;
    const ezAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct ezVariantToProperty
  {
    ezVariantToProperty(const ezVariant& value, const ezAbstractProperty* pProp)
    {
      EZ_IGNORE_UNUSED(pProp);
      m_tempValue = value.ConvertTo<typename ezPropertyValue<T>::StorageType>();
    }

    operator const void*()
    {
      return &m_tempValue;
    }

    typename ezPropertyValue<T>::Type m_tempValue = {};
  };

  template <>
  struct ezVariantToProperty<const char*>
  {
    ezVariantToProperty(const ezVariant& value, const ezAbstractProperty* pProp)
    {
      EZ_IGNORE_UNUSED(pProp);
      m_sData = value.ConvertTo<ezString>();
      m_pValue = m_sData;
    }

    operator const void*()
    {
      return &m_pValue;
    }
    ezString m_sData;
    const char* m_pValue;
  };

  template <>
  struct ezVariantToProperty<ezVariant>
  {
    ezVariantToProperty(const ezVariant& value, const ezAbstractProperty* pProp)
      : m_value(value)
    {
      EZ_IGNORE_UNUSED(pProp);
    }

    operator const void*()
    {
      return const_cast<ezVariant*>(&m_value);
    }

    const ezVariant& m_value;
  };

  template <>
  struct ezVariantToProperty<ezTypedPointer>
  {
    ezVariantToProperty(const ezVariant& value, const ezAbstractProperty* pProp)
    {
      EZ_IGNORE_UNUSED(pProp);

      m_ptr = value.Get<ezTypedPointer>();
      EZ_ASSERT_DEBUG(!m_ptr.m_pType || m_ptr.m_pType->IsDerivedFrom(pProp->GetSpecificType()),
        "Pointer of type '{0}' does not derive from '{}'", m_ptr.m_pType->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }

    operator const void*()
    {
      return &m_ptr.m_pObject;
    }

    ezTypedPointer m_ptr;
  };


  template <>
  struct ezVariantToProperty<ezTypedObject>
  {
    ezVariantToProperty(const ezVariant& value, const ezAbstractProperty* pProp)
    {
      EZ_IGNORE_UNUSED(pProp);
      m_pPtr = value.GetData();
    }

    operator const void*()
    {
      return m_pPtr;
    }
    const void* m_pPtr = nullptr;
  };

  //////////////////////////////////////////////////////////////////////////

  struct GetValueFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()(const ezAbstractMemberProperty* pProp, const void* pObject, ezVariant& value)
    {
      ezVariantFromProperty<T> getter(value, pProp);
      pProp->GetValuePtr(pObject, getter);
    }
  };

  struct SetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->SetValuePtr(pObject, setter);
    }
  };

  struct GetArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex, ezVariant& value)
    {
      ezVariantFromProperty<T> getter(value, pProp);
      pProp->GetValue(pObject, uiIndex, getter);
    }
  };

  struct SetArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->SetValue(pObject, uiIndex, setter);
    }
  };

  struct InsertArrayValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, uiIndex, setter);
    }
  };

  struct InsertSetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, setter);
    }
  };

  struct RemoveSetValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->Remove(pObject, setter);
    }
  };

  struct GetMapValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractMapProperty* pProp, const void* pObject, const char* szKey, ezVariant& value)
    {
      ezVariantFromProperty<T> getter(value, pProp);
      getter.m_bSuccess = pProp->GetValue(pObject, szKey, getter);
    }
  };

  struct SetMapValueFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()(const ezAbstractMapProperty* pProp, void* pObject, const char* szKey, const ezVariant& value)
    {
      ezVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, szKey, setter);
    }
  };

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
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 uiComponent, double fValue)
    {
      EZ_IGNORE_UNUSED(pVector);
      EZ_IGNORE_UNUSED(uiComponent);
      EZ_IGNORE_UNUSED(fValue);
      EZ_ASSERT_DEBUG(false, "ezReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct SetComponentValueImpl<ezVec2Template<T>>
  {
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 uiComponent, double fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 2, "uiComponent out of range");
      auto vec = pVector->Get<ezVec2Template<T>>();
      switch (uiComponent)
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
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 uiComponent, double fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 3, "uiComponent out of range");
      auto vec = pVector->Get<ezVec3Template<T>>();
      switch (uiComponent)
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
    EZ_FORCE_INLINE static void impl(ezVariant* pVector, ezUInt32 uiComponent, double fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 4, "uiComponent out of range");
      auto vec = pVector->Get<ezVec4Template<T>>();
      switch (uiComponent)
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
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 uiComponent, double& out_fValue)
    {
      EZ_IGNORE_UNUSED(pVector);
      EZ_IGNORE_UNUSED(uiComponent);
      EZ_IGNORE_UNUSED(out_fValue);
      EZ_ASSERT_DEBUG(false, "ezReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec2Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 uiComponent, double& out_fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 2, "uiComponent out of range");
      const auto& vec = pVector->Get<ezVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec3Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 uiComponent, double& out_fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 3, "uiComponent out of range");
      const auto& vec = pVector->Get<ezVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<ezVec4Template<T>>
  {
    EZ_FORCE_INLINE static void impl(const ezVariant* pVector, ezUInt32 uiComponent, double& out_fValue)
    {
      EZ_ASSERT_DEBUG(uiComponent < 4, "uiComponent out of range");
      const auto& vec = pVector->Get<ezVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
        case 3:
          out_fValue = static_cast<double>(vec.w);
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
} // namespace

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

bool ezReflectionUtils::IsValueType(const ezAbstractProperty* pProp)
{
  return !pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) || ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProp->GetSpecificType()));
}

const ezRTTI* ezReflectionUtils::GetTypeFromVariant(const ezVariant& value)
{
  return value.GetReflectedType();
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

void ezReflectionUtils::SetComponent(ezVariant& ref_vector, ezUInt32 uiComponent, double fValue)
{
  SetComponentValueFunc func;
  func.m_pVector = &ref_vector;
  func.m_iComponent = uiComponent;
  func.m_fValue = fValue;
  ezVariant::DispatchTo(func, ref_vector.GetType());
}

double ezReflectionUtils::GetComponent(const ezVariant& vector, ezUInt32 uiComponent)
{
  GetComponentValueFunc func;
  func.m_pVector = &vector;
  func.m_iComponent = uiComponent;
  ezVariant::DispatchTo(func, vector.GetType());
  return func.m_fValue;
}

ezVariant ezReflectionUtils::GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject)
{
  ezVariant res;
  EZ_ASSERT_DEBUG(pProp != nullptr, "GetMemberPropertyValue: missing data!");

  GetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, res);

  return res;
}

void ezReflectionUtils::SetMemberPropertyValue(const ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMemberPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Bitflags | ezPropertyFlags::IsEnum))
  {
    auto pEnumerationProp = static_cast<const ezAbstractEnumerationProperty*>(pProp);

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
  }
  else
  {
    SetValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, value);
  }
}

ezVariant ezReflectionUtils::GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex)
{
  ezVariant res;
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    ezLog::Error("GetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    GetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, res);
  }
  return res;
}

void ezReflectionUtils::SetArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    ezLog::Error("SetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    SetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
  }
}

void ezReflectionUtils::InsertSetPropertyValue(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  InsertSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

void ezReflectionUtils::RemoveSetPropertyValue(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  RemoveSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

ezVariant ezReflectionUtils::GetMapPropertyValue(const ezAbstractMapProperty* pProp, const void* pObject, const char* szKey)
{
  ezVariant value;
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetMapPropertyValue: missing data!");

  GetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
  return value;
}

void ezReflectionUtils::SetMapPropertyValue(const ezAbstractMapProperty* pProp, void* pObject, const char* szKey, const ezVariant& value)
{
  EZ_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMapPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  SetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
}

void ezReflectionUtils::InsertArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, const ezVariant& value, ezUInt32 uiIndex)
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

  InsertArrayValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
}

void ezReflectionUtils::RemoveArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex)
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

const ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  ezHybridArray<const ezAbstractProperty*, 32> props;
  pRtti->GetAllProperties(props);
  if (uiPropertyIndex < props.GetCount())
  {
    const ezAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == ezPropertyCategory::Member)
      return static_cast<const ezAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

const ezAbstractMemberProperty* ezReflectionUtils::GetMemberProperty(const ezRTTI* pRtti, const char* szPropertyName)
{
  if (pRtti == nullptr)
    return nullptr;

  if (const ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropertyName))
  {
    if (pProp->GetCategory() == ezPropertyCategory::Member)
      return static_cast<const ezAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

void ezReflectionUtils::GatherTypesDerivedFromClass(const ezRTTI* pBaseRtti, ezSet<const ezRTTI*>& out_types)
{
  ezRTTI::ForEachDerivedType(
    pBaseRtti,
    [&](const ezRTTI* pRtti)
    {
      out_types.Insert(pRtti);
    });
}

void ezReflectionUtils::GatherDependentTypes(const ezRTTI* pRtti, ezSet<const ezRTTI*>& inout_typesAsSet, ezDynamicArray<const ezRTTI*>* out_pTypesAsStack /*= nullptr*/)
{
  auto AddType = [&](const ezRTTI* pNewRtti)
  {
    if (pNewRtti != pRtti && pNewRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType) == false && inout_typesAsSet.Contains(pNewRtti) == false)
    {
      inout_typesAsSet.Insert(pNewRtti);
      if (out_pTypesAsStack != nullptr)
      {
        out_pTypesAsStack->PushBack(pNewRtti);
      }

      GatherDependentTypes(pNewRtti, inout_typesAsSet, out_pTypesAsStack);
    }
  };

  if (const ezRTTI* pParentRtti = pRtti->GetParentType())
  {
    AddType(pParentRtti);
  }

  for (const ezAbstractProperty* prop : pRtti->GetProperties())
  {
    if (prop->GetCategory() == ezPropertyCategory::Constant)
      continue;

    if (prop->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    AddType(prop->GetSpecificType());
  }

  for (const ezAbstractFunctionProperty* func : pRtti->GetFunctions())
  {
    ezUInt32 uiNumArgs = func->GetArgumentCount();
    for (ezUInt32 i = 0; i < uiNumArgs; ++i)
    {
      AddType(func->GetArgumentType(i));
    }
  }

  for (const ezPropertyAttribute* attr : pRtti->GetAttributes())
  {
    AddType(attr->GetDynamicRTTI());
  }
}

ezResult ezReflectionUtils::CreateDependencySortedTypeArray(const ezSet<const ezRTTI*>& types, ezDynamicArray<const ezRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  ezSet<const ezRTTI*> accu;
  ezDynamicArray<const ezRTTI*> tmpStack;

  for (const ezRTTI* pType : types)
  {
    if (accu.Contains(pType))
      continue;

    GatherDependentTypes(pType, accu, &tmpStack);

    while (tmpStack.IsEmpty() == false)
    {
      const ezRTTI* pDependentType = tmpStack.PeekBack();
      EZ_ASSERT_DEBUG(pDependentType != pType, "A type must not be reported as dependency of itself");
      tmpStack.PopBack();

      if (types.Contains(pDependentType) == false)
        return EZ_FAILURE;

      out_sortedTypes.PushBack(pDependentType);
    }

    accu.Insert(pType);
    out_sortedTypes.PushBack(pType);
  }

  EZ_ASSERT_DEV(types.GetCount() == out_sortedTypes.GetCount(), "Not all types have been sorted or the sorted list contains duplicates");
  return EZ_SUCCESS;
}

bool ezReflectionUtils::EnumerationToString(const ezRTTI* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput, ezEnum<EnumConversionMode> conversionMode)
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
          out_sOutput = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
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
          out_sOutput.Append(conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2, "|");
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

void ezReflectionUtils::GetEnumKeysAndValues(const ezRTTI* pEnumerationRtti, ezDynamicArray<EnumKeyValuePair>& ref_entries, ezEnum<EnumConversionMode> conversionMode)
{
  /// \test This is new.

  ref_entries.Clear();

  if (pEnumerationRtti->IsDerivedFrom<ezEnumBase>() || pEnumerationRtti->IsDerivedFrom<ezBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();

        auto& e = ref_entries.ExpandAndGetRef();
        e.m_sKey = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
        e.m_iValue = value.ConvertTo<ezInt32>();
      }
    }
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
        // Testing fully qualified and short value name
        const char* valueNameOnly = ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
        if (ezStringUtils::IsEqual(pProp->GetPropertyName(), szValue) || (valueNameOnly != nullptr && ezStringUtils::IsEqual(valueNameOnly + 2, szValue)))
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
          // Testing fully qualified and short value name
          const char* valueNameOnly = ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
          if (sValue.IsEqual(pProp->GetPropertyName()) || (valueNameOnly != nullptr && sValue.IsEqual(valueNameOnly + 2)))
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
    EZ_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
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
    EZ_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
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
    EZ_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

bool ezReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const ezAbstractProperty* pProp)
{
  // #VAR TEST
  const ezRTTI* pPropType = pProp->GetSpecificType();

  ezVariant vTemp;
  ezVariant vTemp2;

  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const ezAbstractMemberProperty*>(pProp);

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
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) || bIsValueType)
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
      auto pSpecific = static_cast<const ezAbstractArrayProperty*>(pProp);

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
        if (bIsValueType)
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
      auto pSpecific = static_cast<const ezAbstractSetProperty*>(pProp);

      ezHybridArray<ezVariant, 16> values;
      pSpecific->GetValues(pObject, values);
      ezHybridArray<ezVariant, 16> values2;
      pSpecific->GetValues(pObject2, values2);

      const ezUInt32 uiCount = values.GetCount();
      const ezUInt32 uiCount2 = values2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)))
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
      auto pSpecific = static_cast<const ezAbstractMapProperty*>(pProp);

      ezHybridArray<ezString, 16> keys;
      pSpecific->GetKeys(pObject, keys);
      ezHybridArray<ezString, 16> keys2;
      pSpecific->GetKeys(pObject2, keys2);

      const ezUInt32 uiCount = keys.GetCount();
      const ezUInt32 uiCount2 = keys2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)))
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
      else if ((!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)) && pProp->GetFlags().IsSet(ezPropertyFlags::Class))
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
              ezLog::Error("The property '{0}' can not be compared as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
            }
          }
          if (!bEqual)
            break;
        }
        return bEqual;
      }
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
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


void ezReflectionUtils::DeleteObject(void* pObject, const ezAbstractProperty* pOwnerProperty)
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
      return ezVariant(ezMat3::MakeIdentity());
    case ezVariant::Type::Matrix4:
      return ezVariant(ezMat4::MakeIdentity());
    case ezVariant::Type::Transform:
      return ezVariant(ezTransform::MakeIdentity());
    case ezVariant::Type::String:
      return ezVariant(ezString());
    case ezVariant::Type::StringView:
      return ezVariant(ezStringView(), false);
    case ezVariant::Type::DataBuffer:
      return ezVariant(ezDataBuffer());
    case ezVariant::Type::Time:
      return ezVariant(ezTime());
    case ezVariant::Type::Uuid:
      return ezVariant(ezUuid());
    case ezVariant::Type::Angle:
      return ezVariant(ezAngle());
    case ezVariant::Type::HashedString:
      return ezVariant(ezHashedString());
    case ezVariant::Type::TempHashedString:
      return ezVariant(ezTempHashedString());
    case ezVariant::Type::VariantArray:
      return ezVariantArray();
    case ezVariant::Type::VariantDictionary:
      return ezVariantDictionary();
    case ezVariant::Type::TypedPointer:
      return ezVariant(static_cast<void*>(nullptr), nullptr);

    default:
      EZ_REPORT_FAILURE("Invalid case statement");
      return ezVariant();
  }
}

ezVariant ezReflectionUtils::GetDefaultValue(const ezAbstractProperty* pProperty, ezVariant index)
{
  const bool isValueType = ezReflectionUtils::IsValueType(pProperty);
  const ezVariantType::Enum type = pProperty->GetFlags().IsSet(ezPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(ezPropertyFlags::Class) && !isValueType) ? ezVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();
  const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pProperty->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            return pAttrib->GetValue();
          if (pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
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
      else // Class
      {
        return ezUuid();
      }
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<ezVariantArray>())
          {
            if (!index.IsValid())
              return pAttrib->GetValue();

            ezUInt32 iIndex = index.ConvertTo<ezUInt32>();
            const auto& defaultArray = pAttrib->GetValue().Get<ezVariantArray>();
            if (iIndex < defaultArray.GetCount())
            {
              return defaultArray[iIndex];
            }
            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return ezVariantArray();

        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return ezVariantArray();

        return ezUuid();
      }
      break;
    case ezPropertyCategory::Map:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<ezVariantDictionary>())
          {
            if (!index.IsValid())
            {
              return pAttrib->GetValue();
            }
            ezString sKey = index.ConvertTo<ezString>();
            const auto& defaultDict = pAttrib->GetValue().Get<ezVariantDictionary>();
            if (auto it = defaultDict.Find(sKey); it.IsValid())
              return it.Value();

            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return ezVariantDictionary();
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return ezVariantDictionary();

        return ezUuid();
      }
      break;
    default:
      break;
  }

  EZ_REPORT_FAILURE("Don't reach here");
  return ezVariant();
}

ezVariant ezReflectionUtils::GetDefaultVariantFromType(const ezRTTI* pRtti)
{
  ezVariantType::Enum type = pRtti->GetVariantType();
  switch (type)
  {
    case ezVariant::Type::TypedObject:
    {
      ezVariant val;
      val.MoveTypedObject(pRtti->GetAllocator()->Allocate<void>(), pRtti);
      return val;
    }
    break;

    default:
      return GetDefaultVariantFromType(type);
  }
}

void ezReflectionUtils::SetAllMemberPropertiesToDefault(const ezRTTI* pRtti, void* pObject)
{
  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      const ezVariant defValue = ezReflectionUtils::GetDefaultValue(pProp);

      ezReflectionUtils::SetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pObject, defValue);
    }
  }
}

namespace
{
  template <class C>
  struct ezClampCategoryType
  {
    enum
    {
      value = (((ezVariant::TypeDeduction<C>::value >= ezVariantType::Int8 && ezVariant::TypeDeduction<C>::value <= ezVariantType::Double) || (ezVariant::TypeDeduction<C>::value == ezVariantType::Time) || (ezVariant::TypeDeduction<C>::value == ezVariantType::Angle))) + ((ezVariant::TypeDeduction<C>::value >= ezVariantType::Vector2 && ezVariant::TypeDeduction<C>::value <= ezVariantType::Vector4U) * 2)
    };
  };

  template <typename T, int V = ezClampCategoryType<T>::value>
  struct ClampVariantFuncImpl
  {
    static EZ_ALWAYS_INLINE ezResult Func(ezVariant& value, const ezClampValueAttribute* pAttrib)
    {
      EZ_IGNORE_UNUSED(value);
      EZ_IGNORE_UNUSED(pAttrib);
      return EZ_FAILURE;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 1> // scalar types
  {
    static EZ_ALWAYS_INLINE ezResult Func(ezVariant& value, const ezClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = ezMath::Max(value.ConvertTo<T>(), pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = ezMath::Min(value.ConvertTo<T>(), pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return EZ_SUCCESS;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 2> // vector types
  {
    static EZ_ALWAYS_INLINE ezResult Func(ezVariant& value, const ezClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMax(pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMin(pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return EZ_SUCCESS;
    }
  };

  struct ClampVariantFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE ezResult operator()(ezVariant& value, const ezClampValueAttribute* pAttrib)
    {
      return ClampVariantFuncImpl<T>::Func(value, pAttrib);
    }
  };
} // namespace

ezResult ezReflectionUtils::ClampValue(ezVariant& value, const ezClampValueAttribute* pAttrib)
{
  ezVariantType::Enum type = value.GetType();
  if (type == ezVariantType::Invalid || pAttrib == nullptr)
    return EZ_SUCCESS; // If there is nothing to clamp or no clamp attribute we call it a success.

  ClampVariantFunc func;
  return ezVariant::DispatchTo(func, type, value, pAttrib);
}
