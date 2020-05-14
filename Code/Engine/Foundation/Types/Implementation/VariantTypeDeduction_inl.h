
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

  using StorageType = bool;
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

  using StorageType = ezInt8;
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

  using StorageType = ezUInt8;
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

  using StorageType = ezInt16;
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

  using StorageType = ezUInt16;
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

  using StorageType = ezInt32;
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

  using StorageType = ezUInt32;
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

  using StorageType = ezInt64;
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

  using StorageType = ezUInt64;
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

  using StorageType = float;
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

  using StorageType = double;
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

  using StorageType = ezColor;
};

template <>
struct ezVariant::TypeDeduction<ezColorGammaUB>
{
  enum
  {
    value = Type::ColorGamma,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezColorGammaUB;
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

  using StorageType = ezVec2;
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

  using StorageType = ezVec3;
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

  using StorageType = ezVec4;
};

template <>
struct ezVariant::TypeDeduction<ezVec2I32>
{
  enum
  {
    value = Type::Vector2I,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec2I32;
};

template <>
struct ezVariant::TypeDeduction<ezVec3I32>
{
  enum
  {
    value = Type::Vector3I,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec3I32;
};

template <>
struct ezVariant::TypeDeduction<ezVec4I32>
{
  enum
  {
    value = Type::Vector4I,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec4I32;
};

template <>
struct ezVariant::TypeDeduction<ezVec2U32>
{
  enum
  {
    value = Type::Vector2U,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec2U32;
};

template <>
struct ezVariant::TypeDeduction<ezVec3U32>
{
  enum
  {
    value = Type::Vector3U,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec3U32;
};

template <>
struct ezVariant::TypeDeduction<ezVec4U32>
{
  enum
  {
    value = Type::Vector4U,
    forceSharing = false,
    hasReflectedMembers = true
  };

  using StorageType = ezVec4U32;
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

  using StorageType = ezQuat;
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

  using StorageType = ezMat3;
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

  using StorageType = ezMat4;
};

template <>
struct ezVariant::TypeDeduction<ezTransform>
{
  enum
  {
    value = Type::Transform,
    forceSharing = false,
    hasReflectedMembers = false
  };

  using StorageType = ezTransform;
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

  using StorageType = ezString;
};

template <>
struct ezVariant::TypeDeduction<ezUntrackedString>
{
  enum
  {
    value = Type::String,
    forceSharing = true,
    hasReflectedMembers = false
  };

  using StorageType = ezString;
};

template <>
struct ezVariant::TypeDeduction<ezStringView>
{
  enum
  {
    value = Type::StringView,
    forceSharing = false,
    hasReflectedMembers = false
  };

  using StorageType = ezStringView;
};

template <>
struct ezVariant::TypeDeduction<ezDataBuffer>
{
  enum
  {
    value = Type::DataBuffer,
    forceSharing = true,
    hasReflectedMembers = false
  };

  using StorageType = ezDataBuffer;
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

  using StorageType = ezString;
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

  using StorageType = ezString;
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

  using StorageType = ezString;
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

  using StorageType = ezString;
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

  using StorageType = ezTime;
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

  using StorageType = ezUuid;
};

template <>
struct ezVariant::TypeDeduction<ezAngle>
{
  enum
  {
    value = Type::Angle,
    forceSharing = false,
    hasReflectedMembers = false
  };

  using StorageType = ezAngle;
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

  using StorageType = ezVariantArray;
};

template <>
struct ezVariant::TypeDeduction<ezArrayPtr<ezVariant>>
{
  enum
  {
    value = Type::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false
  };

  using StorageType = ezVariantArray;
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

  using StorageType = ezVariantDictionary;
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
    using StorageType = void *;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    using StorageType = ezReflectedClass *;
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

  using StorageType = typename ezInternal::PointerDeductionHelper<(ezConversionTest<const T *, const ezReflectedClass *>::exists && !ezConversionTest<const ezReflectedClass *, const void *>::sameType)>::StorageType;
};

/// \endcond

