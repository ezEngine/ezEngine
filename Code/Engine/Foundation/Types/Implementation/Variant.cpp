#include <FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/VariantTypeRegistry.h>

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
  typedef typename TypeDeduction<ezVariantArray>::StorageType StorageType;
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_Type = TypeDeduction<ezVariantArray>::value;
  m_bIsShared = true;
}

ezVariant::ezVariant(const ezVariantDictionary& value)
{
  typedef typename TypeDeduction<ezVariantDictionary>::StorageType StorageType;
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_Type = TypeDeduction<ezVariantDictionary>::value;
  m_bIsShared = true;
}

ezVariant::ezVariant(const ezTypedPointer& value)
{
  InitInplace(value);
}

ezVariant::ezVariant(const ezTypedObject& value)
{
  void* ptr = ezReflectionSerializer::Clone(value.m_pObject, value.m_pType);
  m_Data.shared = EZ_DEFAULT_NEW(RTTISharedData, ptr, value.m_pType);
  m_Type = Type::TypedObject;
  m_bIsShared = true;
}

void ezVariant::CopyTypedObject(const void* value, const ezRTTI* pType)
{
  Release();
  void* ptr = ezReflectionSerializer::Clone(value, pType);
  m_Data.shared = EZ_DEFAULT_NEW(RTTISharedData, ptr, pType);
  m_Type = Type::TypedObject;
  m_bIsShared = true;
}

void ezVariant::MoveTypedObject(void* value, const ezRTTI* pType)
{
  Release();
  m_Data.shared = EZ_DEFAULT_NEW(RTTISharedData, value, pType);
  m_Type = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
EZ_ALWAYS_INLINE void ezVariant::InitShared(const T& value)
{
  typedef typename TypeDeduction<T>::StorageType StorageType;

  EZ_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  const ezRTTI* pType = ezGetStaticRTTI<T>();

  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_Type = TypeDeduction<T>::value;
  m_bIsShared = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezUInt64 operator()(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(typename ezVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                                  !ezVariant::TypeDeduction<T>::forceSharing,
      "This type requires special handling! Add a specialization below.");
    return ezHashingUtils::xxHash64(pData, sizeof(T), uiSeed);
  }
};

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezString>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  ezString* pString = (ezString*)pData;

  return ezHashingUtils::xxHash64(pString->GetData(), pString->GetElementCount(), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezMat3>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  return ezHashingUtils::xxHash64(pData, sizeof(ezMat3), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezMat4>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  return ezHashingUtils::xxHash64(pData, sizeof(ezMat4), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezTransform>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  return ezHashingUtils::xxHash64(pData, sizeof(ezTransform), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezDataBuffer>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  ezDataBuffer* pDataBuffer = (ezDataBuffer*)pData;

  return ezHashingUtils::xxHash64(pDataBuffer->GetData(), pDataBuffer->GetCount(), uiSeed);
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezVariantArray>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(pData);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezVariantDictionary>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(pData);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezTypedPointer>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(pData);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezTypedObject>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  auto pType = v.GetReflectedType();

  const ezVariantTypeInfo* pTypeInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  EZ_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add EZ_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", pType->GetTypeName());
  ezUInt32 uiHash32 = pTypeInfo->Hash(pData);

  return ezHashingUtils::xxHash64(&uiHash32, sizeof(ezUInt32), uiSeed);
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

template <>
EZ_FORCE_INLINE void CompareFunc::operator()<ezTypedObject>()
{
  m_bResult = false;
  ezTypedObject A = m_pThis->Get<ezTypedObject>();
  ezTypedObject B = m_pOther->Get<ezTypedObject>();
  if (A.m_pType == B.m_pType)
  {
    const ezVariantTypeInfo* pTypeInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(A.m_pType);
    EZ_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add EZ_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", A.m_pType->GetTypeName());
    m_bResult = pTypeInfo->Equal(A.m_pObject, B.m_pObject);
  }
}

struct IndexFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<1>)
  {
    const ezRTTI* pRtti = m_pThis->GetReflectedType();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
    if (!pProp)
      return ezVariant();

    if (m_pThis->GetType() == ezVariantType::TypedPointer)
    {
      const ezTypedPointer& ptr = m_pThis->Get<ezTypedPointer>();
      if (ptr.m_pObject)
        return ezReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return ezVariant();
    }
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
    const ezRTTI* pRtti = m_pThis->GetReflectedType();
    ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_szKey);
    if (!pProp)
      return ezVariant();
    if (m_pThis->GetType() == ezVariantType::TypedPointer)
    {
      const ezTypedPointer& ptr = m_pThis->Get<ezTypedPointer>();
      if (ptr.m_pObject)
        return ezReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return ezVariant();
    }
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

ezTypedPointer ezVariant::GetWriteAccess()
{
  ezTypedPointer obj;
  obj.m_pType = GetReflectedType();
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef > 1)
    {
      // We need to make sure we hold the only reference to the shared data to be able to edit it.
      SharedData* pData = m_Data.shared->Clone();
      Release();
      m_Data.shared = pData;
    }
    obj.m_pObject = m_Data.shared->m_Ptr;
  }
  else
  {
    obj.m_pObject = m_Type == Type::TypedPointer ? Cast<ezTypedPointer>().m_pObject : &m_Data;
  }
  return obj;
}

const ezVariant ezVariant::operator[](ezUInt32 uiIndex) const
{
  if (m_Type == Type::VariantArray)
  {
    const ezVariantArray& a = Cast<ezVariantArray>();
    if (uiIndex < a.GetCount())
      return a[uiIndex];
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

ezVariant ezVariant::operator[](StringWrapper szKey) const
{
  if (m_Type == Type::VariantDictionary)
  {
    ezVariant result;
    Cast<ezVariantDictionary>().TryGetValue(szKey.m_str, result);
    return result;
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

  if (IsVector2Static(type) && (IsVector2Static(m_Type)))
    return true;

  if (IsVector3Static(type) && (IsVector3Static(m_Type)))
    return true;

  if (IsVector4Static(type) && (IsVector4Static(m_Type)))
    return true;

  if (type == Type::String && m_Type < Type::LastStandardType && m_Type != Type::DataBuffer)
    return true;
  if (type == Type::String && m_Type == Type::VariantArray)
    return true;
  if (type == Type::Color && m_Type == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_Type == Type::Color)
    return true;

  if (type == Type::TypedPointer && m_Type == Type::TypedPointer)
    return true;
  if (type == Type::TypedObject && m_Type == Type::TypedObject)
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
  return DispatchTo<ComputeHashFunc>(obj, GetType(), *this, GetData(), uiSeed);
}


inline ezVariant::RTTISharedData::RTTISharedData(void* pData, const ezRTTI* pType)
  : SharedData(pData, pType)
{
  EZ_ASSERT_DEBUG(pType != nullptr && pType->GetAllocator()->CanAllocate(), "");
}

inline ezVariant::RTTISharedData::~RTTISharedData()
{
  m_pType->GetAllocator()->Deallocate(m_Ptr);
}


ezVariant::ezVariant::SharedData* ezVariant::RTTISharedData::Clone() const
{
  void* ptr = ezReflectionSerializer::Clone(m_Ptr, m_pType);
  return EZ_DEFAULT_NEW(RTTISharedData, ptr, m_pType);
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
EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<ezTypedPointer>()
{
  m_pType = m_pVariant->Cast<ezTypedPointer>().m_pType;
}
template <>
EZ_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<ezTypedObject>()
{
  m_pType = m_pVariant->m_bIsShared ? m_pVariant->m_Data.shared->m_pType : m_pVariant->m_Data.inlined.m_pType;
}

const ezRTTI* ezVariant::GetReflectedType() const
{
  if (m_Type != Type::Invalid)
  {
    GetTypeFromVariantFunc func;
    func.m_pVariant = this;
    func.m_pType = nullptr;
    ezVariant::DispatchTo(func, GetType());
    return func.m_pType;
  }
  return nullptr;
}

void ezVariant::InitTypedPointer(void* value, const ezRTTI* pType)
{
  ezTypedPointer ptr;
  ptr.m_pObject = value;
  ptr.m_pType = pType;

  ezMemoryUtils::CopyConstruct(reinterpret_cast<ezTypedPointer*>(&m_Data), ptr, 1);

  m_Type = TypeDeduction<ezTypedPointer>::value;
  m_bIsShared = false;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Variant);
