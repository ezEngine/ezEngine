
#define EZ_MSVC_WARNING_NUMBER 4702 // Unreachable code for some reason
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

EZ_ALWAYS_INLINE ezVariant::ezVariant()
{
  m_uiType = Type::Invalid;
  m_bIsShared = false;
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVariant& other)
{
  CopyFrom(other);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(ezVariant&& other) noexcept
{
  MoveFrom(std::move(other));
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const bool& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezInt8& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezUInt8& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezInt16& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezUInt16& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezInt32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezUInt32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezInt64& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezUInt64& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const float& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const double& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezColor& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec2& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec3& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec4& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec2I32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec3I32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec4I32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec2U32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec3U32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezVec4U32& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezQuat& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezTime& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezUuid& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezAngle& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezColorGammaUB& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezHashedString& value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezTempHashedString& value)
{
  InitInplace(value);
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int>>
EZ_ALWAYS_INLINE ezVariant::ezVariant(const T& value)
{
  const constexpr bool forceSharing = TypeDeduction<T>::forceSharing;
  const constexpr bool inlineSized = sizeof(T) <= InlinedStruct::DataSize;
  const constexpr bool isPOD = ezIsPodType<T>::value;
  InitTypedObject(value, ezTraitInt < (!forceSharing && inlineSized && isPOD) ? 1 : 0 > ());
}

template <typename T>
EZ_ALWAYS_INLINE ezVariant::ezVariant(const T* value)
{
  constexpr bool bla = !std::is_same<T, void>::value;
  EZ_CHECK_AT_COMPILETIME(bla);
  InitTypedPointer(const_cast<T*>(value), ezGetStaticRTTI<T>());
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(void* value, const ezRTTI* pType)
{
  InitTypedPointer(value, pType);
}

EZ_ALWAYS_INLINE ezVariant::~ezVariant()
{
  Release();
}

EZ_ALWAYS_INLINE void ezVariant::operator=(const ezVariant& other)
{
  if (this != &other)
  {
    Release();
    CopyFrom(other);
  }
}

EZ_ALWAYS_INLINE void ezVariant::operator=(ezVariant&& other) noexcept
{
  if (this != &other)
  {
    Release();
    MoveFrom(std::move(other));
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezVariant::operator=(const T& value)
{
  *this = ezVariant(value);
}

EZ_ALWAYS_INLINE bool ezVariant::operator!=(const ezVariant& other) const
{
  return !(*this == other);
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::operator==(const T& other) const
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  struct TypeInfo
  {
    enum
    {
      isNumber = TypeDeduction<T>::value > Type::Invalid&& TypeDeduction<T>::value <= Type::Double
    };
  };

  if (IsFloatingPoint())
  {
    return ezVariantHelper::CompareFloat(*this, other, ezTraitInt<TypeInfo::isNumber>());
  }
  else if (IsNumber())
  {
    return ezVariantHelper::CompareNumber(*this, other, ezTraitInt<TypeInfo::isNumber>());
  }

  EZ_ASSERT_DEV(IsA<StorageType>(), "Stored type '{0}' does not match comparison type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<StorageType>() == other;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezVariant::operator!=(const T& other) const
{
  return !(*this == other);
}

EZ_ALWAYS_INLINE bool ezVariant::IsValid() const
{
  return m_uiType != Type::Invalid;
}

EZ_ALWAYS_INLINE bool ezVariant::IsNumber() const
{
  return IsNumberStatic(m_uiType);
}

EZ_ALWAYS_INLINE bool ezVariant::IsFloatingPoint() const
{
  return IsFloatingPointStatic(m_uiType);
}

EZ_ALWAYS_INLINE bool ezVariant::IsString() const
{
  return IsStringStatic(m_uiType);
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int>>
EZ_ALWAYS_INLINE bool ezVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int>>
EZ_ALWAYS_INLINE bool ezVariant::IsA() const
{
  if (m_uiType == TypeDeduction<T>::value)
  {
    const ezTypedPointer& ptr = *reinterpret_cast<const ezTypedPointer*>(&m_Data);
    // Always allow cast to void*.
    if constexpr (std::is_same<T, void*>::value || std::is_same<T, const void*>::value)
    {
      return true;
    }
    else if (ptr.m_pType)
    {
      using NonPointerT = typename ezTypeTraits<T>::NonConstReferencePointerType;
      const ezRTTI* pType = ezGetStaticRTTI<NonPointerT>();
      return IsDerivedFrom(ptr.m_pType, pType);
    }
    else if (!ptr.m_pObject)
    {
      // nullptr can be converted to anything
      return true;
    }
  }
  return false;
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int>>
EZ_ALWAYS_INLINE bool ezVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int>>
EZ_ALWAYS_INLINE bool ezVariant::IsA() const
{
  using NonRefT = typename ezTypeTraits<T>::NonConstReferenceType;
  if (m_uiType == TypeDeduction<T>::value)
  {
    if (const ezRTTI* pType = GetReflectedType())
    {
      return IsDerivedFrom(pType, ezGetStaticRTTI<NonRefT>());
    }
  }
  return false;
}

EZ_ALWAYS_INLINE ezVariant::Type::Enum ezVariant::GetType() const
{
  return static_cast<Type::Enum>(m_uiType);
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int>>
EZ_ALWAYS_INLINE const T& ezVariant::Get() const
{
  EZ_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int>>
EZ_ALWAYS_INLINE T ezVariant::Get() const
{
  EZ_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int>>
EZ_ALWAYS_INLINE const T ezVariant::Get() const
{
  EZ_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int>>
EZ_ALWAYS_INLINE const T& ezVariant::Get() const
{
  EZ_ASSERT_DEV(m_uiType == TypeDeduction<T>::value, "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int>>
EZ_ALWAYS_INLINE T& ezVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int>>
EZ_ALWAYS_INLINE T ezVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T>(Get<T>());
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int>>
EZ_ALWAYS_INLINE T& ezVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

EZ_ALWAYS_INLINE const void* ezVariant::GetData() const
{
  if (m_uiType == Type::TypedPointer)
  {
    return Cast<ezTypedPointer>().m_pObject;
  }
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezVariant::CanConvertTo() const
{
  return CanConvertTo(static_cast<Type::Enum>(TypeDeduction<T>::value));
}

template <typename T>
T ezVariant::ConvertTo(ezResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo<T>())
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_FAILURE;

    return T();
  }

  if (m_uiType == TypeDeduction<T>::value)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_SUCCESS;

    return Cast<T>();
  }

  T result = {};
  bool bSuccessful = true;
  ezVariantHelper::To(*this, result, bSuccessful);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = bSuccessful ? EZ_SUCCESS : EZ_FAILURE;

  return result;
}


/// private methods

template <typename T>
EZ_FORCE_INLINE void ezVariant::InitInplace(const T& value)
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  EZ_CHECK_AT_COMPILETIME_MSG(ezGetTypeClass<T>::value <= ezTypeIsMemRelocatable::value, "in place data needs to be POD or mem relocatable");
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(T) <= sizeof(m_Data), "value of this type is too big to bestored inline in a Variant");
  ezMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);

  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = false;
}

template <typename T>
EZ_FORCE_INLINE void ezVariant::InitTypedObject(const T& value, ezTraitInt<0>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  EZ_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(InlinedStruct::DataSize)) || TypeDeduction<T>::forceSharing, "Value should be inplace instead.");
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  const ezRTTI* pType = ezGetStaticRTTI<T>();
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
EZ_FORCE_INLINE void ezVariant::InitTypedObject(const T& value, ezTraitInt<1>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  EZ_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) <= InlinedStruct::DataSize) && !TypeDeduction<T>::forceSharing, "Value can't be stored inplace.");
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  EZ_CHECK_AT_COMPILETIME_MSG(ezIsPodType<T>::value, "in place data needs to be POD");
  ezMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_Data.inlined.m_pType = ezGetStaticRTTI<T>();
  m_uiType = Type::TypedObject;
  m_bIsShared = false;
}

inline void ezVariant::Release()
{
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef.Decrement() == 0)
    {
      EZ_DEFAULT_DELETE(m_Data.shared);
    }
  }
}

inline void ezVariant::CopyFrom(const ezVariant& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;

  if (m_bIsShared)
  {
    m_Data.shared = other.m_Data.shared;
    m_Data.shared->m_uiRef.Increment();
  }
  else if (other.IsValid())
  {
    m_Data = other.m_Data;
  }
}

EZ_ALWAYS_INLINE void ezVariant::MoveFrom(ezVariant&& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;
  m_Data = other.m_Data;

  other.m_uiType = Type::Invalid;
  other.m_bIsShared = false;
  other.m_Data.shared = nullptr;
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int>>
const T& ezVariant::Cast() const
{
  const bool validType = ezConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  EZ_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int>>
T ezVariant::Cast() const
{
  const ezTypedPointer& ptr = *reinterpret_cast<const ezTypedPointer*>(&m_Data);

  const ezRTTI* pType = GetReflectedType();
  using NonRefPtrT = typename ezTypeTraits<T>::NonConstReferencePointerType;
  if constexpr (!std::is_same<T, void*>::value && !std::is_same<T, const void*>::value)
  {
    EZ_ASSERT_DEV(pType == nullptr || IsDerivedFrom(pType, ezGetStaticRTTI<NonRefPtrT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(ezGetStaticRTTI<NonRefPtrT>()));
  }
  return static_cast<T>(ptr.m_pObject);
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int>>
const T ezVariant::Cast() const
{
  ezTypedObject obj;
  obj.m_pObject = GetData();
  obj.m_pType = GetReflectedType();
  return obj;
}

template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int>>
const T& ezVariant::Cast() const
{
  const ezRTTI* pType = GetReflectedType();
  using NonRefT = typename ezTypeTraits<T>::NonConstReferenceType;
  EZ_ASSERT_DEV(IsDerivedFrom(pType, ezGetStaticRTTI<NonRefT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(ezGetStaticRTTI<NonRefT>()));

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

EZ_ALWAYS_INLINE bool ezVariant::IsNumberStatic(ezUInt32 type)
{
  return type > Type::FirstStandardType && type <= Type::Double;
}

EZ_ALWAYS_INLINE bool ezVariant::IsFloatingPointStatic(ezUInt32 type)
{
  return type == Type::Float || type == Type::Double;
}

EZ_ALWAYS_INLINE bool ezVariant::IsStringStatic(ezUInt32 type)
{
  return type == Type::String || type == Type::StringView;
}

EZ_ALWAYS_INLINE bool ezVariant::IsVector2Static(ezUInt32 type)
{
  return type == Type::Vector2 || type == Type::Vector2I || type == Type::Vector2U;
}

EZ_ALWAYS_INLINE bool ezVariant::IsVector3Static(ezUInt32 type)
{
  return type == Type::Vector3 || type == Type::Vector3I || type == Type::Vector3U;
}

EZ_ALWAYS_INLINE bool ezVariant::IsVector4Static(ezUInt32 type)
{
  return type == Type::Vector4 || type == Type::Vector4I || type == Type::Vector4U;
}

template <typename T>
T ezVariant::ConvertNumber() const
{
  switch (m_uiType)
  {
    case Type::Bool:
      return static_cast<T>(Cast<bool>());
    case Type::Int8:
      return static_cast<T>(Cast<ezInt8>());
    case Type::UInt8:
      return static_cast<T>(Cast<ezUInt8>());
    case Type::Int16:
      return static_cast<T>(Cast<ezInt16>());
    case Type::UInt16:
      return static_cast<T>(Cast<ezUInt16>());
    case Type::Int32:
      return static_cast<T>(Cast<ezInt32>());
    case Type::UInt32:
      return static_cast<T>(Cast<ezUInt32>());
    case Type::Int64:
      return static_cast<T>(Cast<ezInt64>());
    case Type::UInt64:
      return static_cast<T>(Cast<ezUInt64>());
    case Type::Float:
      return static_cast<T>(Cast<float>());
    case Type::Double:
      return static_cast<T>(Cast<double>());
  }

  EZ_REPORT_FAILURE("Variant is not a number");
  return T(0);
}

template <>
struct ezHashHelper<ezVariant>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezVariant& value)
  {
    ezUInt64 uiHash = value.ComputeHash(0);
    return (ezUInt32)uiHash;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezVariant& a, const ezVariant& b) { return a == b; }
};
