
/// \cond

template <>
struct ezVariant::TypeDeduction<bool>
{
  enum
  {
    value = Type::Bool,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef bool StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt8>
{
  enum
  {
    value = Type::Int8,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezInt8 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt8>
{
  enum
  {
    value = Type::UInt8,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezUInt8 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt16>
{
  enum
  {
    value = Type::Int16,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezInt16 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt16>
{
  enum
  {
    value = Type::UInt16,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezUInt16 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt32>
{
  enum
  {
    value = Type::Int32,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezInt32 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt32>
{
  enum
  {
    value = Type::UInt32,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezUInt32 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt64>
{
  enum
  {
    value = Type::Int64,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezInt64 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt64>
{
  enum
  {
    value = Type::UInt64,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezUInt64 StorageType;
};

template <>
struct ezVariant::TypeDeduction<float>
{
  enum
  {
    value = Type::Float,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef float StorageType;
};

template <>
struct ezVariant::TypeDeduction<double>
{
  enum
  {
    value = Type::Double,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef double StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezColor>
{
  enum
  {
    value = Type::Color,
    forceSharing = false,
    hasReflectedMembers = true
  };

  typedef ezColor StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVec2>
{
  enum
  {
    value = Type::Vector2,
    forceSharing = false,
    hasReflectedMembers = true
  };

  typedef ezVec2 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVec3>
{
  enum
  {
    value = Type::Vector3,
    forceSharing = false,
    hasReflectedMembers = true
  };

  typedef ezVec3 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVec4>
{
  enum
  {
    value = Type::Vector4,
    forceSharing = false,
    hasReflectedMembers = true
  };

  typedef ezVec4 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezQuat>
{
  enum
  {
    value = Type::Quaternion,
    forceSharing = false,
    hasReflectedMembers = true
  };

  typedef ezQuat StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezMat3>
{
  enum
  {
    value = Type::Matrix3,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezMat3 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezMat4>
{
  enum
  {
    value = Type::Matrix4,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezMat4 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezString>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<char*>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };
  
  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<const char*>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezString StorageType;
};

template <size_t N>
struct ezVariant::TypeDeduction<char[N]>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };
  
  typedef ezString StorageType;
};

template <size_t N>
struct ezVariant::TypeDeduction<const char[N]>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezTime>
{
  enum
  {
    value = Type::Time,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezTime StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUuid>
{
  enum
  {
    value = Type::Uuid,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef ezUuid StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVariantArray>
{
  enum
  {
    value = Type::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezVariantArray StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezArrayPtr<ezVariant> >
{
  enum
  {
    value = Type::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezVariantArray StorageType;
};


template <>
struct ezVariant::TypeDeduction<ezVariantDictionary>
{
  enum
  {
    value = Type::VariantDictionary,
    forceSharing = true,
    hasReflectedMembers = false
  };

  typedef ezVariantDictionary StorageType;
};

namespace ezInternal
{
  template <int v>
  struct PointerDeductionHelper
  {
  };

  template <>
  struct PointerDeductionHelper<0>
  {
    typedef void* StorageType;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    typedef ezReflectedClass* StorageType;
  };
}

template <typename T>
struct ezVariant::TypeDeduction<T*>
{
  enum
  {
    value = EZ_IS_DERIVED_FROM_STATIC(ezReflectedClass, T) ? Type::ReflectedPointer : Type::VoidPointer,
    forceSharing = false,
    hasReflectedMembers = false
  };

  typedef typename ezInternal::PointerDeductionHelper<EZ_IS_DERIVED_FROM_STATIC(ezReflectedClass, T)>::StorageType StorageType;
};

/// \endcond

