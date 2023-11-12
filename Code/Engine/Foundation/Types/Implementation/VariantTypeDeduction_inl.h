


/// \cond

template <>
struct ezVariantTypeDeduction<bool>
{
  enum
  {
    value = ezVariantType::Bool,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = bool;
  using ReturnType = bool;
};

template <>
struct ezVariantTypeDeduction<ezInt8>
{
  enum
  {
    value = ezVariantType::Int8,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezInt8;
};

template <>
struct ezVariantTypeDeduction<ezUInt8>
{
  enum
  {
    value = ezVariantType::UInt8,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezUInt8;
};

template <>
struct ezVariantTypeDeduction<ezInt16>
{
  enum
  {
    value = ezVariantType::Int16,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezInt16;
};

template <>
struct ezVariantTypeDeduction<ezUInt16>
{
  enum
  {
    value = ezVariantType::UInt16,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezUInt16;
};

template <>
struct ezVariantTypeDeduction<ezInt32>
{
  enum
  {
    value = ezVariantType::Int32,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezInt32;
};

template <>
struct ezVariantTypeDeduction<ezUInt32>
{
  enum
  {
    value = ezVariantType::UInt32,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezUInt32;
};

template <>
struct ezVariantTypeDeduction<ezInt64>
{
  enum
  {
    value = ezVariantType::Int64,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezInt64;
};

template <>
struct ezVariantTypeDeduction<ezUInt64>
{
  enum
  {
    value = ezVariantType::UInt64,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezUInt64;
};

template <>
struct ezVariantTypeDeduction<float>
{
  enum
  {
    value = ezVariantType::Float,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = float;
};

template <>
struct ezVariantTypeDeduction<double>
{
  enum
  {
    value = ezVariantType::Double,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = double;
};

template <>
struct ezVariantTypeDeduction<ezColor>
{
  enum
  {
    value = ezVariantType::Color,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezColor;
};

template <>
struct ezVariantTypeDeduction<ezColorGammaUB>
{
  enum
  {
    value = ezVariantType::ColorGamma,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezColorGammaUB;
};

template <>
struct ezVariantTypeDeduction<ezVec2>
{
  enum
  {
    value = ezVariantType::Vector2,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec2;
};

template <>
struct ezVariantTypeDeduction<ezVec3>
{
  enum
  {
    value = ezVariantType::Vector3,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec3;
};

template <>
struct ezVariantTypeDeduction<ezVec4>
{
  enum
  {
    value = ezVariantType::Vector4,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec4;
};

template <>
struct ezVariantTypeDeduction<ezVec2I32>
{
  enum
  {
    value = ezVariantType::Vector2I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec2I32;
};

template <>
struct ezVariantTypeDeduction<ezVec3I32>
{
  enum
  {
    value = ezVariantType::Vector3I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec3I32;
};

template <>
struct ezVariantTypeDeduction<ezVec4I32>
{
  enum
  {
    value = ezVariantType::Vector4I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec4I32;
};

template <>
struct ezVariantTypeDeduction<ezVec2U32>
{
  enum
  {
    value = ezVariantType::Vector2U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec2U32;
};

template <>
struct ezVariantTypeDeduction<ezVec3U32>
{
  enum
  {
    value = ezVariantType::Vector3U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec3U32;
};

template <>
struct ezVariantTypeDeduction<ezVec4U32>
{
  enum
  {
    value = ezVariantType::Vector4U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVec4U32;
};

template <>
struct ezVariantTypeDeduction<ezQuat>
{
  enum
  {
    value = ezVariantType::Quaternion,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezQuat;
};

template <>
struct ezVariantTypeDeduction<ezMat3>
{
  enum
  {
    value = ezVariantType::Matrix3,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezMat3;
};

template <>
struct ezVariantTypeDeduction<ezMat4>
{
  enum
  {
    value = ezVariantType::Matrix4,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezMat4;
};

template <>
struct ezVariantTypeDeduction<ezTransform>
{
  enum
  {
    value = ezVariantType::Transform,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezTransform;
};

template <>
struct ezVariantTypeDeduction<ezString>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezUntrackedString>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezStringView>
{
  enum
  {
    value = ezVariantType::StringView,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezStringView;
};

template <>
struct ezVariantTypeDeduction<ezDataBuffer>
{
  enum
  {
    value = ezVariantType::DataBuffer,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezDataBuffer;
};

template <>
struct ezVariantTypeDeduction<char*>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<const char*>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <size_t N>
struct ezVariantTypeDeduction<char[N]>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <size_t N>
struct ezVariantTypeDeduction<const char[N]>
{
  enum
  {
    value = ezVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezTime>
{
  enum
  {
    value = ezVariantType::Time,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezTime;
};

template <>
struct ezVariantTypeDeduction<ezUuid>
{
  enum
  {
    value = ezVariantType::Uuid,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezUuid;
};

template <>
struct ezVariantTypeDeduction<ezAngle>
{
  enum
  {
    value = ezVariantType::Angle,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezAngle;
};

template <>
struct ezVariantTypeDeduction<ezHashedString>
{
  enum
  {
    value = ezVariantType::HashedString,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezHashedString;
};

template <>
struct ezVariantTypeDeduction<ezTempHashedString>
{
  enum
  {
    value = ezVariantType::TempHashedString,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezTempHashedString;
};

template <>
struct ezVariantTypeDeduction<ezVariantArray>
{
  enum
  {
    value = ezVariantType::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVariantArray;
};

template <>
struct ezVariantTypeDeduction<ezArrayPtr<ezVariant>>
{
  enum
  {
    value = ezVariantType::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezVariantArray;
};


template <>
struct ezVariantTypeDeduction<ezVariantDictionary>
{
  enum
  {
    value = ezVariantType::VariantDictionary,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = ezVariantClass::DirectCast
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
    using StorageType = void*;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    using StorageType = ezReflectedClass*;
  };
} // namespace ezInternal

template <>
struct ezVariantTypeDeduction<ezTypedPointer>
{
  enum
  {
    value = ezVariantType::TypedPointer,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::DirectCast
  };

  using StorageType = ezTypedPointer;
};

template <typename T>
struct ezVariantTypeDeduction<T*>
{
  enum
  {
    value = ezVariantType::TypedPointer,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::PointerCast
  };

  using StorageType = ezTypedPointer;
};

template <>
struct ezVariantTypeDeduction<ezTypedObject>
{
  enum
  {
    value = ezVariantType::TypedObject,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = ezVariantClass::TypedObject
  };

  using StorageType = ezTypedObject;
};

/// \endcond
