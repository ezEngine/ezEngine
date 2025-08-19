#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/VariantTypeRegistry.h>

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
static_assert(sizeof(ezVariant) == 24);
#else
static_assert(sizeof(ezVariant) == 20);
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

ezVariant::ezVariant(const ezStringView& value, bool bCopyString)
{
  if (bCopyString)
    InitShared(ezString(value));
  else
    InitInplace(value);
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
  using StorageType = typename TypeDeduction<ezVariantArray>::StorageType;
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<ezVariantArray>::value;
  m_bIsShared = true;
}

ezVariant::ezVariant(const ezVariantDictionary& value)
{
  using StorageType = typename TypeDeduction<ezVariantDictionary>::StorageType;
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<ezVariantDictionary>::value;
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
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void ezVariant::CopyTypedObject(const void* value, const ezRTTI* pType)
{
  Release();
  void* ptr = ezReflectionSerializer::Clone(value, pType);
  m_Data.shared = EZ_DEFAULT_NEW(RTTISharedData, ptr, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void ezVariant::MoveTypedObject(void* value, const ezRTTI* pType)
{
  Release();
  m_Data.shared = EZ_DEFAULT_NEW(RTTISharedData, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
EZ_ALWAYS_INLINE void ezVariant::InitShared(const T& value)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  static_assert((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  static_assert(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  const ezRTTI* pType = ezGetStaticRTTI<T>();

  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezUInt64 operator()(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
  {
    EZ_IGNORE_UNUSED(v);
    static_assert(sizeof(typename ezVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                    !ezVariant::TypeDeduction<T>::forceSharing,
      "This type requires special handling! Add a specialization below.");
    return ezHashingUtils::xxHash64(pData, sizeof(T), uiSeed);
  }
};

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezString>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  auto pString = static_cast<const ezString*>(pData);
  return ezHashingUtils::xxHash64String(*pString, uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezMat3>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  return ezHashingUtils::xxHash64(pData, sizeof(ezMat3), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezMat4>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  return ezHashingUtils::xxHash64(pData, sizeof(ezMat4), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezTransform>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  return ezHashingUtils::xxHash64(pData, sizeof(ezTransform), uiSeed);
}

template <>
EZ_ALWAYS_INLINE ezUInt64 ComputeHashFunc::operator()<ezDataBuffer>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  auto pDataBuffer = static_cast<const ezDataBuffer*>(pData);
  return ezHashingUtils::xxHash64(pDataBuffer->GetData(), pDataBuffer->GetCount(), uiSeed);
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezVariantArray>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);

  auto pVariantArray = static_cast<const ezVariantArray*>(pData);

  ezUInt64 uiHash = uiSeed;
  for (const ezVariant& var : *pVariantArray)
  {
    uiHash = var.ComputeHash(uiHash);
  }

  return uiHash;
}

template <>
ezUInt64 ComputeHashFunc::operator()<ezVariantDictionary>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);

  auto pVariantDictionary = static_cast<const ezVariantDictionary*>(pData);

  ezHybridArray<ezUInt64, 128> hashes;
  hashes.Reserve(pVariantDictionary->GetCount() * 2);

  for (auto& it : *pVariantDictionary)
  {
    hashes.PushBack(ezHashingUtils::xxHash64String(it.Key(), uiSeed));
    hashes.PushBack(it.Value().ComputeHash(uiSeed));
  }

  hashes.Sort();

  return ezHashingUtils::xxHash64(hashes.GetData(), hashes.GetCount() * sizeof(ezUInt64), uiSeed);
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezTypedPointer>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  EZ_IGNORE_UNUSED(v);
  EZ_IGNORE_UNUSED(pData);
  EZ_IGNORE_UNUSED(uiSeed);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
EZ_FORCE_INLINE ezUInt64 ComputeHashFunc::operator()<ezTypedObject>(const ezVariant& v, const void* pData, ezUInt64 uiSeed)
{
  auto pType = v.GetReflectedType();

  const ezVariantTypeInfo* pTypeInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  EZ_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add EZ_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", pType->GetTypeName());
  EZ_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
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
    EZ_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
    m_bResult = pTypeInfo->Equal(A.m_pObject, B.m_pObject);
  }
}

struct IndexFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezVariant Impl(ezTraitInt<1>)
  {
    const ezRTTI* pRtti = m_pThis->GetReflectedType();
    const ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
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
    const ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRtti, m_szKey);
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
    T result = {};
    ezVariantHelper::To(*m_pThis, result, m_bSuccessful);

    if constexpr (std::is_same_v<T, ezStringView>)
    {
      m_Result = ezVariant(result, false);
    }
    else
    {
      m_Result = result;
    }
  }

  const ezVariant* m_pThis;
  ezVariant m_Result;
  bool m_bSuccessful;
};

/// public methods

bool ezVariant::operator==(const ezVariant& other) const
{
  if (m_uiType == Type::Invalid && other.m_uiType == Type::Invalid)
  {
    return true;
  }
  else if ((IsFloatingPoint() && other.IsNumber()) || (other.IsFloatingPoint() && IsNumber()))
  {
    // if either of them is a floating point number, compare them as doubles

    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber() && other.IsNumber())
  {
    return ConvertNumber<ezInt64>() == other.ConvertNumber<ezInt64>();
  }
  else if (IsString() && other.IsString())
  {
    const ezStringView a = IsA<ezStringView>() ? Get<ezStringView>() : ezStringView(Get<ezString>().GetData());
    const ezStringView b = other.IsA<ezStringView>() ? other.Get<ezStringView>() : ezStringView(other.Get<ezString>().GetData());
    return a.IsEqual(b);
  }
  else if (IsHashedString() && other.IsHashedString())
  {
    const ezTempHashedString a = IsA<ezTempHashedString>() ? Get<ezTempHashedString>() : ezTempHashedString(Get<ezHashedString>());
    const ezTempHashedString b = other.IsA<ezTempHashedString>() ? other.Get<ezTempHashedString>() : ezTempHashedString(other.Get<ezHashedString>());
    return a == b;
  }
  else if (m_uiType == other.m_uiType)
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
    obj.m_pObject = m_uiType == Type::TypedPointer ? Cast<ezTypedPointer>().m_pObject : &m_Data;
  }
  return obj;
}

const ezVariant ezVariant::operator[](ezUInt32 uiIndex) const
{
  if (m_uiType == Type::VariantArray)
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

const ezVariant ezVariant::operator[](StringWrapper key) const
{
  if (m_uiType == Type::VariantDictionary)
  {
    ezVariant result;
    Cast<ezVariantDictionary>().TryGetValue(key.m_str, result);
    return result;
  }
  else if (IsValid())
  {
    KeyFunc func;
    func.m_pThis = this;
    func.m_szKey = key.m_str;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return ezVariant();
}

bool ezVariant::CanConvertTo(Type::Enum type) const
{
  if (m_uiType == type)
    return true;

  if (type == Type::Invalid)
    return false;

  const bool bTargetIsString = (type == Type::String) || (type == Type::HashedString) || (type == Type::TempHashedString);

  if (bTargetIsString && m_uiType == Type::Invalid)
    return true;

  if (bTargetIsString && (m_uiType > Type::FirstStandardType && m_uiType < Type::LastStandardType && m_uiType != Type::DataBuffer))
    return true;
  if (bTargetIsString && (m_uiType == Type::VariantArray || m_uiType == Type::VariantDictionary))
    return true;
  if (type == Type::StringView && (m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;
  if (type == Type::TempHashedString && m_uiType == Type::HashedString)
    return true;

  if (!IsValid())
    return false;

  if (IsNumberStatic(type) && (IsNumber() || m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;

  if (IsVector2Static(type) && (IsVector2Static(m_uiType) || IsNumber()))
    return true;

  if (IsVector3Static(type) && (IsVector3Static(m_uiType) || IsNumber()))
    return true;

  if (IsVector4Static(type) && (IsVector4Static(m_uiType) || IsNumber()))
    return true;

  if (type == Type::Color && m_uiType == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_uiType == Type::Color)
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

  if (m_uiType == type)
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
  return DispatchTo<ComputeHashFunc>(obj, GetType(), *this, GetData(), uiSeed + GetType());
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
  if (m_uiType != Type::Invalid)
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

  m_uiType = TypeDeduction<ezTypedPointer>::value;
  m_bIsShared = false;
}

bool ezVariant::IsDerivedFrom(const ezRTTI* pType1, const ezRTTI* pType2)
{
  return pType1->IsDerivedFrom(pType2);
}

ezStringView ezVariant::GetTypeName(const ezRTTI* pType)
{
  return pType->GetTypeName();
}

//////////////////////////////////////////////////////////////////////////

struct AddFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()(const ezVariant& a, const ezVariant& b, ezVariant& out_res)
  {
    if constexpr (std::is_same_v<T, ezInt8> || std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt16> || std::is_same_v<T, ezUInt16> ||
                  std::is_same_v<T, ezInt32> || std::is_same_v<T, ezUInt32> ||
                  std::is_same_v<T, ezInt64> || std::is_same_v<T, ezUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezVec2> || std::is_same_v<T, ezVec3> || std::is_same_v<T, ezVec4> ||
                  std::is_same_v<T, ezVec2I32> || std::is_same_v<T, ezVec3I32> || std::is_same_v<T, ezVec4I32> ||
                  std::is_same_v<T, ezVec2U32> || std::is_same_v<T, ezVec3U32> || std::is_same_v<T, ezVec4U32> ||
                  std::is_same_v<T, ezTime> ||
                  std::is_same_v<T, ezAngle>)
    {
      out_res = a.Get<T>() + b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, ezString> || std::is_same_v<T, ezStringView>)
    {
      ezStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());
      out_res = ezString(s.GetView());
    }
    else if constexpr (std::is_same_v<T, ezHashedString>)
    {
      ezStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());

      ezHashedString hashedS;
      hashedS.Assign(s);
      out_res = hashedS;
    }
  }
};

ezVariant operator+(const ezVariant& a, const ezVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = ezMath::Max(a.GetType(), b.GetType());

    AddFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    AddFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return ezVariant();
}

//////////////////////////////////////////////////////////////////////////

struct SubFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()(const ezVariant& a, const ezVariant& b, ezVariant& out_res)
  {
    if constexpr (std::is_same_v<T, ezInt8> || std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt16> || std::is_same_v<T, ezUInt16> ||
                  std::is_same_v<T, ezInt32> || std::is_same_v<T, ezUInt32> ||
                  std::is_same_v<T, ezInt64> || std::is_same_v<T, ezUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezVec2> || std::is_same_v<T, ezVec3> || std::is_same_v<T, ezVec4> ||
                  std::is_same_v<T, ezVec2I32> || std::is_same_v<T, ezVec3I32> || std::is_same_v<T, ezVec4I32> ||
                  std::is_same_v<T, ezVec2U32> || std::is_same_v<T, ezVec3U32> || std::is_same_v<T, ezVec4U32> ||
                  std::is_same_v<T, ezTime> ||
                  std::is_same_v<T, ezAngle>)
    {
      out_res = a.Get<T>() - b.Get<T>();
    }
  }
};

ezVariant operator-(const ezVariant& a, const ezVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = ezMath::Max(a.GetType(), b.GetType());

    SubFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    SubFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return ezVariant();
}

//////////////////////////////////////////////////////////////////////////

struct MulFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()(const ezVariant& a, const ezVariant& b, ezVariant& out_res)
  {
    if constexpr (std::is_same_v<T, ezInt8> || std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt16> || std::is_same_v<T, ezUInt16> ||
                  std::is_same_v<T, ezInt32> || std::is_same_v<T, ezUInt32> ||
                  std::is_same_v<T, ezInt64> || std::is_same_v<T, ezUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, ezColor> ||
                  std::is_same_v<T, ezTime>)
    {
      out_res = a.Get<T>() * b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, ezVec2> || std::is_same_v<T, ezVec3> || std::is_same_v<T, ezVec4> ||
                       std::is_same_v<T, ezVec2I32> || std::is_same_v<T, ezVec3I32> || std::is_same_v<T, ezVec4I32> ||
                       std::is_same_v<T, ezVec2U32> || std::is_same_v<T, ezVec3U32> || std::is_same_v<T, ezVec4U32>)
    {
      out_res = a.Get<T>().CompMul(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, ezAngle>)
    {
      out_res = ezAngle(a.Get<T>() * b.Get<T>().GetRadian());
    }
  }
};

ezVariant operator*(const ezVariant& a, const ezVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = ezMath::Max(a.GetType(), b.GetType());

    MulFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    MulFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return ezVariant();
}

//////////////////////////////////////////////////////////////////////////

struct DivFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()(const ezVariant& a, const ezVariant& b, ezVariant& out_res)
  {
    if constexpr (std::is_same_v<T, ezInt8> || std::is_same_v<T, ezUInt8> ||
                  std::is_same_v<T, ezInt16> || std::is_same_v<T, ezUInt16> ||
                  std::is_same_v<T, ezInt32> || std::is_same_v<T, ezUInt32> ||
                  std::is_same_v<T, ezInt64> || std::is_same_v<T, ezUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, ezTime>)
    {
      out_res = a.Get<T>() / b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, ezVec2> || std::is_same_v<T, ezVec3> || std::is_same_v<T, ezVec4> ||
                       std::is_same_v<T, ezVec2I32> || std::is_same_v<T, ezVec3I32> || std::is_same_v<T, ezVec4I32> ||
                       std::is_same_v<T, ezVec2U32> || std::is_same_v<T, ezVec3U32> || std::is_same_v<T, ezVec4U32>)
    {
      out_res = a.Get<T>().CompDiv(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, ezAngle>)
    {
      out_res = ezAngle(a.Get<T>() / b.Get<T>().GetRadian());
    }
  }
};

ezVariant operator/(const ezVariant& a, const ezVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = ezMath::Max(a.GetType(), b.GetType());

    DivFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    DivFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return ezVariant();
}

//////////////////////////////////////////////////////////////////////////

struct LerpFunc
{
  constexpr static bool CanInterpolate(ezVariantType::Enum variantType)
  {
    return variantType >= ezVariantType::Int8 && variantType <= ezVariantType::Vector4;
  }

  template <typename T>
  EZ_ALWAYS_INLINE void operator()(const ezVariant& a, const ezVariant& b, double x, ezVariant& out_res)
  {
    if constexpr (std::is_same_v<T, ezQuat>)
    {
      ezQuat q = ezQuat::MakeSlerp(a.Get<ezQuat>(), b.Get<ezQuat>(), static_cast<float>(x));
      out_res = q;
    }
    else if constexpr (CanInterpolate(static_cast<ezVariantType::Enum>(ezVariantTypeDeduction<T>::value)))
    {
      out_res = ezMath::Lerp(a.Get<T>(), b.Get<T>(), static_cast<float>(x));
    }
    else
    {
      out_res = (x < 0.5) ? a : b;
    }
  }
};

namespace ezMath
{
  ezVariant Lerp(const ezVariant& a, const ezVariant& b, double fFactor)
  {
    LerpFunc func;
    ezVariant result;
    ezVariant::DispatchTo(func, a.GetType(), a, b, fFactor, result);
    return result;
  }
} // namespace ezMath
