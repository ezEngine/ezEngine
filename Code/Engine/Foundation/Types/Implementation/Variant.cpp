#include <FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 24);
#else
EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 20);
#endif

/// constructors

ezVariant::ezVariant(const ezMat3& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezMat4& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezTransform& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const char* value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezString& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezUntrackedString& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezDataBuffer& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezVariantArray& value)
{
  InitShared(value);
}

ezVariant::ezVariant(const ezVariantDictionary& value)
{
  InitShared(value);
}

template <typename T>
EZ_ALWAYS_INLINE void ezVariant::InitShared(const T& value)
{
  typedef typename TypeDeduction<T>::StorageType StorageType;

  EZ_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");

  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value);
  m_Type = TypeDeduction<T>::value;
  m_bIsShared = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(typename ezVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                                    !ezVariant::TypeDeduction<T>::forceSharing,
                                "This type requires special handling! Add a specialization below.");
    m_uiHash = ezHashingUtils::xxHash64(m_pData, sizeof(T), m_uiHash);
  }

  const void* m_pData;
  ezUInt64 m_uiHash;
};

template <>
EZ_ALWAYS_INLINE void ComputeHashFunc::operator()<ezString>()
{
  ezString* pData = (ezString*)m_pData;

  m_uiHash = ezHashingUtils::xxHash64(pData->GetData(), pData->GetElementCount(), m_uiHash);
}

template <>
EZ_ALWAYS_INLINE void ComputeHashFunc::operator()<ezMat3>()
{
  m_uiHash = ezHashingUtils::xxHash64(m_pData, sizeof(ezMat3), m_uiHash);
}

template <>
EZ_ALWAYS_INLINE void ComputeHashFunc::operator()<ezMat4>()
{
  m_uiHash = ezHashingUtils::xxHash64(m_pData, sizeof(ezMat4), m_uiHash);
}

template <>
EZ_ALWAYS_INLINE void ComputeHashFunc::operator()<ezTransform>()
{
  m_uiHash = ezHashingUtils::xxHash64(m_pData, sizeof(ezTransform), m_uiHash);
}

template <>
EZ_ALWAYS_INLINE void ComputeHashFunc::operator()<ezDataBuffer>()
{
  ezDataBuffer* pData = (ezDataBuffer*)m_pData;

  m_uiHash = ezHashingUtils::xxHash64(pData->GetData(), pData->GetCount(), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator()<ezVariantArray>()
{
  ezVariantArray* pData = (ezVariantArray*)m_pData;
  EZ_IGNORE_UNUSED(pData);

  EZ_ASSERT_NOT_IMPLEMENTED;
  // m_uiHash = ezHashingUtils::xxHash64(pData, sizeof(ezMat4), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator()<ezVariantDictionary>()
{
  ezVariantDictionary* pData = (ezVariantDictionary*)m_pData;
  EZ_IGNORE_UNUSED(pData);

  EZ_ASSERT_NOT_IMPLEMENTED;
  // m_uiHash = ezHashingUtils::xxHash64(pData, sizeof(ezMat4), m_uiHash);
}

struct CompareFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()()
  {
    m_bResult = m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const ezVariant* m_pThis;
  const ezVariant* m_pOther;
  bool m_bResult;
};

struct IndexFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<1>)
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<T>();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
    if (!pProp)
      return ezVariant();
    return ezReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  EZ_ALWAYS_INLINE ezVariant Impl(ezTraitInt<0>)
  {
    return ezVariant();
  }

  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    m_Result = Impl<T>(ezTraitInt<ezVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const ezVariant* m_pThis;
  ezVariant m_Result;
  ezUInt32 m_uiIndex;
};

struct KeyFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<1>)
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<T>();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_szKey);
    if (!pProp)
      return ezVariant();
    return ezReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  EZ_ALWAYS_INLINE ezVariant Impl(ezTraitInt<0>)
  {
    return ezVariant();
  }

  template <typename T>
  EZ_ALWAYS_INLINE void operator()()
  {
    m_Result = Impl<T>(ezTraitInt<ezVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const ezVariant* m_pThis;
  ezVariant m_Result;
  const char* m_szKey;
};

struct ConvertFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()()
  {
    T result;
    ezVariantHelper::To(*m_pThis, result, m_bSuccessful);
    m_Result = result;
  }

  const ezVariant* m_pThis;
  ezVariant m_Result;
  bool m_bSuccessful;
};

/// public methods

bool ezVariant::operator==(const ezVariant& other) const
{
  if (m_Type == Type::Invalid && other.m_Type == Type::Invalid)
  {
    return true;
  }
  else if (IsFloatingPoint() && other.IsNumber())
  {
    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber() && other.IsNumber())
  {
    return ConvertNumber<ezInt64>() == other.ConvertNumber<ezInt64>();
  }
  else if (m_Type == other.m_Type)
  {
    CompareFunc compareFunc;
    compareFunc.m_pThis = this;
    compareFunc.m_pOther = &other;

    DispatchTo(compareFunc, GetType());

    return compareFunc.m_bResult;
  }

  return false;
}

ezVariant ezVariant::operator[](ezUInt32 uiIndex) const
{
  if (m_Type == Type::VariantArray)
  {
    const ezVariantArray& a = Cast<ezVariantArray>();
    if (uiIndex < a.GetCount())
      return a[uiIndex];
  }
  else if (m_Type == Type::ReflectedPointer)
  {
    ezReflectedClass* pObject = Cast<ezReflectedClass*>();
    const ezRTTI* pRtti = pObject->GetDynamicRTTI();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, uiIndex);
    if (!pProp)
      return ezVariant();
    return ezReflectionUtils::GetMemberPropertyValue(pProp, pObject);
  }
  else if (IsValid())
  {
    IndexFunc func;
    func.m_pThis = this;
    func.m_uiIndex = uiIndex;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return ezVariant();
}

ezVariant ezVariant::operator[](ezHashingUtils::StringWrapper szKey) const
{
  if (m_Type == Type::VariantDictionary)
  {
    ezVariant result;
    Cast<ezVariantDictionary>().TryGetValue(szKey.m_str, result);
    return result;
  }
  else if (m_Type == Type::ReflectedPointer)
  {
    ezReflectedClass* pObject = Cast<ezReflectedClass*>();
    const ezRTTI* pRtti = pObject->GetDynamicRTTI();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, szKey.m_str);
    if (!pProp)
      return ezVariant();
    return ezReflectionUtils::GetMemberPropertyValue(pProp, pObject);
  }
  else if (IsValid())
  {
    KeyFunc func;
    func.m_pThis = this;
    func.m_szKey = szKey.m_str;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return ezVariant();
}

bool ezVariant::CanConvertTo(Type::Enum type) const
{
  if (m_Type == type)
    return true;

  if (!IsValid() || type == Type::Invalid)
    return false;

  if (IsNumberStatic(type) && (IsNumber() || m_Type == Type::String))
    return true;

  if (type == Type::String && m_Type < Type::LastStandardType && m_Type != Type::DataBuffer)
    return true;
  if (type == Type::String && m_Type == Type::VariantArray)
    return true;
  if (type == Type::Color && m_Type == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_Type == Type::Color)
    return true;

  if (type == Type::VoidPointer && m_Type == Type::ReflectedPointer)
    return true;

  return false;
}

ezVariant ezVariant::ConvertTo(Type::Enum type, ezResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo(type))
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_FAILURE;

    return ezVariant(); // creates an invalid variant
  }

  if (m_Type == type)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_SUCCESS;

    return *this;
  }

  ConvertFunc convertFunc;
  convertFunc.m_pThis = this;
  convertFunc.m_bSuccessful = true;

  DispatchTo(convertFunc, type);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = convertFunc.m_bSuccessful ? EZ_SUCCESS : EZ_FAILURE;

  return convertFunc.m_Result;
}

ezUInt64 ezVariant::ComputeHash(ezUInt64 uiSeed) const
{
  if (!IsValid())
    return uiSeed;

  ComputeHashFunc obj;
  obj.m_uiHash = uiSeed;
  obj.m_pData = GetData();

  DispatchTo<ComputeHashFunc>(obj, GetType());

  return obj.m_uiHash;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Variant);

