
#define EZ_MSVC_WARNING_NUMBER 4702 // Unreachable code for some reason
#include <Foundation/Basics/Compiler/DisableWarning.h>

EZ_ALWAYS_INLINE ezVariant::ezVariant()
{
  m_Type = Type::Invalid;
  m_bIsShared = false;
}

#include <Foundation/Basics/Compiler/RestoreWarning.h>

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

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezStringView& value)
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

EZ_ALWAYS_INLINE ezVariant::ezVariant(ezReflectedClass* value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const ezReflectedClass* value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(void* value)
{
  InitInplace(value);
}

EZ_ALWAYS_INLINE ezVariant::ezVariant(const void* value)
{
  InitInplace(value);
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

  EZ_ASSERT_DEV(IsA<StorageType>(), "Stored type '{0}' does not match comparison type '{1}'", m_Type, TypeDeduction<T>::value);
  return Cast<StorageType>() == other;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezVariant::operator!=(const T& other) const
{
  return !(*this == other);
}

EZ_ALWAYS_INLINE bool ezVariant::IsValid() const
{
  return m_Type != Type::Invalid;
}

EZ_ALWAYS_INLINE bool ezVariant::IsNumber() const
{
  return IsNumberStatic(m_Type);
}

EZ_ALWAYS_INLINE bool ezVariant::IsFloatingPoint() const
{
  return IsFloatingPointStatic(m_Type);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezVariant::IsA() const
{
  return m_Type == TypeDeduction<T>::value;
}

EZ_ALWAYS_INLINE ezVariant::Type::Enum ezVariant::GetType() const
{
  return static_cast<Type::Enum>(m_Type);
}

template <typename T>
EZ_ALWAYS_INLINE const T& ezVariant::Get() const
{
  EZ_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_Type, TypeDeduction<T>::value);
  return Cast<T>();
}

EZ_ALWAYS_INLINE void* ezVariant::GetData()
{
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

EZ_ALWAYS_INLINE const void* ezVariant::GetData() const
{
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

  if (m_Type == TypeDeduction<T>::value)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_SUCCESS;

    return Cast<T>();
  }

  T result;
  bool bSuccessful = true;
  ezVariantHelper::To(*this, result, bSuccessful);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = bSuccessful ? EZ_SUCCESS : EZ_FAILURE;

  return result;
}

// for some reason MSVC does not accept the template keyword here
#if EZ_ENABLED(EZ_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) functor.operator()<type>()
#else
#  define CALL_FUNCTOR(functor, type) functor.template operator()<type>()
#endif

template <typename Functor>
void ezVariant::DispatchTo(Functor& functor, Type::Enum type)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(functor, ezInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(functor, ezUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(functor, ezInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(functor, ezUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(functor, ezInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(functor, ezUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(functor, ezInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(functor, ezUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(functor, ezColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(functor, ezColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(functor, ezVec2);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(functor, ezVec3);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(functor, ezVec4);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(functor, ezVec2I32);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(functor, ezVec3I32);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(functor, ezVec4I32);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(functor, ezVec2U32);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(functor, ezVec3U32);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(functor, ezVec4U32);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(functor, ezQuat);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(functor, ezMat3);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(functor, ezMat4);
      break;

    case Type::Transform:
      CALL_FUNCTOR(functor, ezTransform);
      break;

    case Type::String:
      CALL_FUNCTOR(functor, ezString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(functor, ezStringView);
      break;

    case Type::DataBuffer:
      CALL_FUNCTOR(functor, ezDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(functor, ezTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(functor, ezUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(functor, ezAngle);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(functor, ezVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(functor, ezVariantDictionary);
      break;

    case Type::ReflectedPointer:
      CALL_FUNCTOR(functor, ezReflectedClass*);
      break;

    case Type::VoidPointer:
      CALL_FUNCTOR(functor, void*);
      break;

    default:
      EZ_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      break;
  }
}

#undef CALL_FUNCTOR

/// private methods

template <typename T>
EZ_FORCE_INLINE void ezVariant::InitInplace(const T& value)
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  EZ_CHECK_AT_COMPILETIME_MSG(ezIsPodType<T>::value, "in place data needs to be POD");
  ezMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);

  m_Type = TypeDeduction<T>::value;
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
  m_Type = other.m_Type;
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
  m_Type = other.m_Type;
  m_bIsShared = other.m_bIsShared;
  m_Data = other.m_Data;

  other.m_Type = Type::Invalid;
  other.m_bIsShared = false;
  other.m_Data.shared = nullptr;
}

template <typename T>
EZ_FORCE_INLINE T& ezVariant::Cast()
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "Value of this type cannot be compared against a Variant");
  const bool validType = ezConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  EZ_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ? *static_cast<T*>(m_Data.shared->m_Ptr)
                                                                      : *reinterpret_cast<T*>(&m_Data);
}

template <typename T>
EZ_FORCE_INLINE const T& ezVariant::Cast() const
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "Value of this type cannot be compared against a Variant");
  const bool validType = ezConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  EZ_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ? *static_cast<const T*>(m_Data.shared->m_Ptr)
                                                                      : *reinterpret_cast<const T*>(&m_Data);
}

EZ_ALWAYS_INLINE bool ezVariant::IsNumberStatic(ezUInt32 type)
{
  return type > Type::Invalid && type <= Type::Double;
}

EZ_ALWAYS_INLINE bool ezVariant::IsFloatingPointStatic(ezUInt32 type)
{
  return type == Type::Float || type == Type::Double;
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
  switch (m_Type)
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
