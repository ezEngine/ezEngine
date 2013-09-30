
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

EZ_FORCE_INLINE bool ezVariant::operator!=(const ezVariant& other) const
{
  return !(*this == other);
}

template <typename T>
EZ_FORCE_INLINE bool ezVariant::operator==(const T& other) const
{
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
  return CanConvertTo(TypeDeduction<T>::value);
}

template <typename T>
EZ_FORCE_INLINE T ezVariant::ConvertTo() const
{
  T result;
  ezVariantConversion::To(result, *this);
  return result;
}

template <typename R, typename Functor>
R ezVariant::DispatchTo(Functor& functor, Type::Enum type)
{
  switch (type)
  {
  case Type::Bool:
    return functor.operator()<bool>();

  case Type::Int32:
    return functor.operator()<ezInt32>();

  case Type::UInt32:
    return functor.operator()<ezUInt32>();

  case Type::Int64:
    return functor.operator()<ezInt64>();

  case Type::UInt64:
    return functor.operator()<ezUInt64>();

  case Type::Float:
    return functor.operator()<float>();

  case Type::Double:
    return functor.operator()<double>();

  /*case Type::Color:
    return functor.operator()<ezColor>();*/

  case Type::Vector2:
    return functor.operator()<ezVec2>();

  case Type::Vector3:
    return functor.operator()<ezVec3>();

  case Type::Vector4:
    return functor.operator()<ezVec4>();

  case Type::Quaternion:
    return functor.operator()<ezQuat>();

  case Type::Matrix3:
    return functor.operator()<ezMat3>();

  case Type::Matrix4:
    return functor.operator()<ezMat4>();

  case Type::String:
    return functor.operator()<ezString>();

  case Type::Time:
    return functor.operator()<ezTime>();

  case Type::VariantArray:
    return functor.operator()<ezVariantArray>();

  case Type::VariantDictionary:
    return functor.operator()<ezVariantDictionary>();

  /*case Type::ObjectPointer:
    return functor.operator()<ezObject*>();*/

  case Type::VoidPointer:
    return functor.operator()<void*>();

  }

  EZ_REPORT_FAILURE("Could not dispatch type '%d'", type);
  return R();
}

/// private methods

template <typename T>
EZ_FORCE_INLINE void ezVariant::Init(const T& value)
{
  EZ_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");

  m_Type = TypeDeduction<T>::value;

  Store<TypeDeduction<T>::StorageType, T>(value, ezTraitInt<(sizeof(TypeDeduction<T>::StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing>());
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
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_SAME_TYPE(T, TypeDeduction<T>::StorageType), "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ?
    *static_cast<T*>(m_Data.shared->m_Ptr) :
    *reinterpret_cast<T*>(&m_Data);
}

template <typename T>
EZ_FORCE_INLINE const T& ezVariant::Cast() const
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_SAME_TYPE(T, TypeDeduction<T>::StorageType), "Invalid Cast, can only cast to storage type");

  return (sizeof(T) > sizeof(Data) || TypeDeduction<T>::forceSharing) ?
    *static_cast<const T*>(m_Data.shared->m_Ptr) :
    *reinterpret_cast<const T*>(&m_Data);
}

template <typename T>
T ezVariant::ConvertNumber() const
{
  switch (m_Type)
  {
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
