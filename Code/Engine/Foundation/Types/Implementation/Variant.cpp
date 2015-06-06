#include <Foundation/PCH.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 24);
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 20);
#endif

/// functors

struct ComputeHashFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(typename ezVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                                !ezVariant::TypeDeduction<T>::forceSharing, "This type requires special handling! Add a specialization below.");
    m_uiHash = ezHashing::MurmurHash64(m_pData, sizeof(T), m_uiHash);
  }

  const void* m_pData;
  ezUInt64 m_uiHash;
};

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator() < ezString > ()
{
  ezString* pData = (ezString*) m_pData;

  m_uiHash = ezHashing::MurmurHash64(pData->GetData(), pData->GetElementCount(), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator() < ezMat3 > ()
{
  ezMat3* pData = (ezMat3*) m_pData;

  m_uiHash = ezHashing::MurmurHash64(pData, sizeof(ezMat3), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator() < ezMat4 > ()
{
  ezMat4* pData = (ezMat4*) m_pData;

  m_uiHash = ezHashing::MurmurHash64(pData, sizeof(ezMat4), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator() < ezVariantArray > ()
{
  ezVariantArray* pData = (ezVariantArray*) m_pData;

  EZ_ASSERT_NOT_IMPLEMENTED;
  //m_uiHash = ezHashing::MurmurHash64(pData, sizeof(ezMat4), m_uiHash);
}

template <>
EZ_FORCE_INLINE void ComputeHashFunc::operator() < ezVariantDictionary > ()
{
  ezVariantDictionary* pData = (ezVariantDictionary*) m_pData;

  EZ_ASSERT_NOT_IMPLEMENTED;
  //m_uiHash = ezHashing::MurmurHash64(pData, sizeof(ezMat4), m_uiHash);
}

struct CompareFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
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
    return ezReflectionUtils::GetMemberPropertyValue(ezReflectionUtils::GetMemberProperty(pRtti, m_uiIndex), m_pThis->GetData());
  }

  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<0>)
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
    return ezReflectionUtils::GetMemberPropertyValue(ezReflectionUtils::GetMemberProperty(pRtti, m_szKey), m_pThis->GetData());
  }

  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<0>)
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
  const char* m_szKey;
};

struct ConvertFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
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
  else if (IsFloatingPoint(m_Type) && IsNumber(other.m_Type))
  {
    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber(m_Type) && IsNumber(other.m_Type))
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
    return ezReflectionUtils::GetMemberPropertyValue(ezReflectionUtils::GetMemberProperty(pRtti, uiIndex), pObject);
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

ezVariant ezVariant::operator[](ezHashing::StringWrapper szKey) const
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
    return ezReflectionUtils::GetMemberPropertyValue(ezReflectionUtils::GetMemberProperty(pRtti, szKey.m_str), pObject);
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

  if (IsNumber(type) && (IsNumber(m_Type) || m_Type == Type::String))
    return true;

  if (type == Type::String && m_Type < Type::VariantArray)
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

