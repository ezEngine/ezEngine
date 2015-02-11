
EZ_FORCE_INLINE ezVariant::ezVariant()
{
  m_Type = Type::Invalid;
  m_bIsShared = false;
}

EZ_FORCE_INLINE ezVariant::ezVariant(const ezVariant& other)
{
  CopyFrom(other);
}

EZ_FORCE_INLINE ezVariant::ezVariant(ezVariant&& other)
{
  MoveFrom(std::move(other));
}

template <typename T>
EZ_FORCE_INLINE ezVariant::ezVariant(const T& value)
{
  Init(value);
}

EZ_FORCE_INLINE ezVariant::~ezVariant() 
{
  Release();
}

EZ_FORCE_INLINE void ezVariant::operator=(const ezVariant& other)
{
  if (this != &other)
  {
    Release();
    CopyFrom(other);
  }
}

EZ_FORCE_INLINE void ezVariant::operator=(ezVariant&& other)
{
  if (this != &other)
  {
    Release();
    MoveFrom(std::move(other));
  }
}

template <typename T>
EZ_FORCE_INLINE void ezVariant::operator=(const T& value)
{
  Release();
  Init(value);
}

EZ_FORCE_INLINE bool ezVariant::operator!=(const ezVariant& other) const
{
  return !(*this == other);
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::operator==(const T& other) const
{
  struct TypeInfo
  {
    enum
    {
      isNumber = TypeDeduction<T>::value > Type::Invalid && TypeDeduction<T>::value <= Type::Double
    };
  };

  if (IsFloatingPoint(m_Type))
  {
    return ezVariantHelper::CompareFloat(*this, other, ezTraitInt<TypeInfo::isNumber>());
  }
  else if (IsNumber(m_Type))
  {
    return ezVariantHelper::CompareNumber(*this, other, ezTraitInt<TypeInfo::isNumber>());
  }

  EZ_ASSERT_DEV(IsA<T>(), "Stored type '%d' does not match comparison type '%d'", m_Type, TypeDeduction<T>::value);
  return Cast<T>() == other;
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::operator!=(const T& other) const
{
  return !(*this == other);
}

EZ_FORCE_INLINE bool ezVariant::IsValid() const
{
  return m_Type != Type::Invalid;
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::IsA() const
{
  return m_Type == TypeDeduction<T>::value;
}

EZ_FORCE_INLINE ezVariant::Type::Enum ezVariant::GetType() const
{
  return static_cast<Type::Enum>(m_Type);
}

template <typename T>
EZ_FORCE_INLINE const T& ezVariant::Get() const
{
  EZ_ASSERT_DEV(IsA<T>(), "Stored type '%d' does not match requested type '%d'", m_Type, TypeDeduction<T>::value);
  return Cast<T>();
}

EZ_FORCE_INLINE void* ezVariant::GetData()
{
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

EZ_FORCE_INLINE const void* ezVariant::GetData() const
{
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::CanConvertTo() const
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
#if EZ_ENABLED(EZ_COMPILER_MSVC)
  #define CALL_FUNCTOR(functor, type) functor.operator()<type>()
#else
  #define CALL_FUNCTOR(functor, type) functor.template operator()<type>()
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

  case Type::Vector2:
    CALL_FUNCTOR(functor, ezVec2);
    break;

  case Type::Vector3:
    CALL_FUNCTOR(functor, ezVec3);
    break;

  case Type::Vector4:
    CALL_FUNCTOR(functor, ezVec4);
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

  case Type::String:
    CALL_FUNCTOR(functor, ezString);
    break;

  case Type::Time:
    CALL_FUNCTOR(functor, ezTime);
    break;

  case Type::Uuid:
    CALL_FUNCTOR(functor, ezUuid);
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
    EZ_REPORT_FAILURE("Could not dispatch type '%d'", type);
    break;
  }  
}

#undef CALL_FUNCTOR

/// private methods

template <typename T>
EZ_FORCE_INLINE void ezVariant::Init(const T& value)
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");

  m_Type = TypeDeduction<T>::value;

  Store<typename TypeDeduction<T>::StorageType, T>(value, ezTraitInt<(sizeof(typename TypeDeduction<T>::StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing>());
}

template <typename StorageType, typename T>
EZ_FORCE_INLINE void ezVariant::Store(const T& value, ezTraitInt<0>)
{
  EZ_CHECK_AT_COMPILETIME_MSG(ezIsPodType<T>::value, "in place data needs to be POD");
  ezMemoryUtils::Construct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_bIsShared = false;
}

template <typename StorageType, typename T>
EZ_FORCE_INLINE void ezVariant::Store(const T& value, ezTraitInt<1>)
{
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>)(value);
  m_bIsShared = true;
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

EZ_FORCE_INLINE void ezVariant::MoveFrom(ezVariant&& other)
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

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ?
    *static_cast<T*>(m_Data.shared->m_Ptr) :
    *reinterpret_cast<T*>(&m_Data);
}

template <typename T>
EZ_FORCE_INLINE const T& ezVariant::Cast() const
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "Value of this type cannot be compared against a Variant");
  const bool validType = ezConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  EZ_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ?
    *static_cast<const T*>(m_Data.shared->m_Ptr) :
    *reinterpret_cast<const T*>(&m_Data);
}

EZ_FORCE_INLINE bool ezVariant::IsNumber(ezUInt32 type)
{
  return type > Type::Invalid && type <= Type::Double;
}

EZ_FORCE_INLINE bool ezVariant::IsFloatingPoint(ezUInt32 type)
{
  return type == Type::Float || type == Type::Double;
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

