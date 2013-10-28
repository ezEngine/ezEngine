
EZ_FORCE_INLINE ezVariant::ezVariant()
{
  m_Type = Type::Invalid;
  m_bIsShared = false;
}

EZ_FORCE_INLINE ezVariant::ezVariant(const ezVariant& other)
{
  CopyFrom(other);
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

  EZ_ASSERT(IsA<T>(), "Stored type '%d' does not match comparison type '%d'", m_Type, TypeDeduction<T>::value);
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
  EZ_ASSERT(IsA<T>(), "Stored type '%d' does not match requested type '%d'", m_Type, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::CanConvertTo() const
{
  return CanConvertTo(static_cast<Type::Enum>(TypeDeduction<T>::value));
}

template <typename T>
EZ_FORCE_INLINE T ezVariant::ConvertTo() const
{
  T result;
  ezVariantConversion::To(result, *this);
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

  /*case Type::Color:
    CALL_FUNCTOR(functor, ezColor);
    break;*/

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

  case Type::VariantArray:
    CALL_FUNCTOR(functor, ezVariantArray);
    break;

  case Type::VariantDictionary:
    CALL_FUNCTOR(functor, ezVariantDictionary);
    break;

  /*case Type::ObjectPointer:
    CALL_FUNCTOR(functor, ezObject*);
    break;*/

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
  ezMemoryUtils::Construct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_bIsShared = false;
}

template <typename StorageType, typename T>
EZ_FORCE_INLINE void ezVariant::Store(const T& value, ezTraitInt<1>)
{
  m_Data.shared = EZ_DEFAULT_NEW(TypedSharedData<StorageType>)(value);
  m_bIsShared = true;
}

template <typename T>
EZ_FORCE_INLINE T& ezVariant::Cast()
{
  const bool validType = ezConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  EZ_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ?
    *static_cast<T*>(m_Data.shared->m_Ptr) :
    *reinterpret_cast<T*>(&m_Data);
}

template <typename T>
EZ_FORCE_INLINE const T& ezVariant::Cast() const
{
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
