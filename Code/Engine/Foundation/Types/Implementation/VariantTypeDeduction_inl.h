


/// \cond

template <>
struct ezVariantTypeDeduction<bool>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Bool;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = bool;
  using ReturnType = bool;
};

template <>
struct ezVariantTypeDeduction<ezInt8>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Int8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezInt8;
};

template <>
struct ezVariantTypeDeduction<ezUInt8>
{
  static constexpr ezVariantType::Enum value = ezVariantType::UInt8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezUInt8;
};

template <>
struct ezVariantTypeDeduction<ezInt16>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Int16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezInt16;
};

template <>
struct ezVariantTypeDeduction<ezUInt16>
{
  static constexpr ezVariantType::Enum value = ezVariantType::UInt16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezUInt16;
};

template <>
struct ezVariantTypeDeduction<ezInt32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Int32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezInt32;
};

template <>
struct ezVariantTypeDeduction<ezUInt32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::UInt32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezUInt32;
};

template <>
struct ezVariantTypeDeduction<ezInt64>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Int64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezInt64;
};

template <>
struct ezVariantTypeDeduction<ezUInt64>
{
  static constexpr ezVariantType::Enum value = ezVariantType::UInt64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezUInt64;
};

template <>
struct ezVariantTypeDeduction<float>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Float;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = float;
};

template <>
struct ezVariantTypeDeduction<double>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Double;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = double;
};

template <>
struct ezVariantTypeDeduction<ezColor>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Color;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezColor;
};

template <>
struct ezVariantTypeDeduction<ezColorGammaUB>
{
  static constexpr ezVariantType::Enum value = ezVariantType::ColorGamma;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezColorGammaUB;
};

template <>
struct ezVariantTypeDeduction<ezVec2>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector2;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec2;
};

template <>
struct ezVariantTypeDeduction<ezVec3>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec3;
};

template <>
struct ezVariantTypeDeduction<ezVec4>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec4;
};

template <>
struct ezVariantTypeDeduction<ezVec2I32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector2I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec2I32;
};

template <>
struct ezVariantTypeDeduction<ezVec3I32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector3I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec3I32;
};

template <>
struct ezVariantTypeDeduction<ezVec4I32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector4I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec4I32;
};

template <>
struct ezVariantTypeDeduction<ezVec2U32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector2U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec2U32;
};

template <>
struct ezVariantTypeDeduction<ezVec3U32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector3U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec3U32;
};

template <>
struct ezVariantTypeDeduction<ezVec4U32>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Vector4U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVec4U32;
};

template <>
struct ezVariantTypeDeduction<ezQuat>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Quaternion;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezQuat;
};

template <>
struct ezVariantTypeDeduction<ezMat3>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Matrix3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezMat3;
};

template <>
struct ezVariantTypeDeduction<ezMat4>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Matrix4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezMat4;
};

template <>
struct ezVariantTypeDeduction<ezTransform>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Transform;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezTransform;
};

template <>
struct ezVariantTypeDeduction<ezString>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezUntrackedString>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezStringView>
{
  static constexpr ezVariantType::Enum value = ezVariantType::StringView;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezStringView;
};

template <>
struct ezVariantTypeDeduction<ezDataBuffer>
{
  static constexpr ezVariantType::Enum value = ezVariantType::DataBuffer;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezDataBuffer;
};

template <>
struct ezVariantTypeDeduction<char*>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<const char*>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <size_t N>
struct ezVariantTypeDeduction<char[N]>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <size_t N>
struct ezVariantTypeDeduction<const char[N]>
{
  static constexpr ezVariantType::Enum value = ezVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezString;
};

template <>
struct ezVariantTypeDeduction<ezTime>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Time;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezTime;
};

template <>
struct ezVariantTypeDeduction<ezUuid>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Uuid;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezUuid;
};

template <>
struct ezVariantTypeDeduction<ezAngle>
{
  static constexpr ezVariantType::Enum value = ezVariantType::Angle;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezAngle;
};

template <>
struct ezVariantTypeDeduction<ezHashedString>
{
  static constexpr ezVariantType::Enum value = ezVariantType::HashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezHashedString;
};

template <>
struct ezVariantTypeDeduction<ezTempHashedString>
{
  static constexpr ezVariantType::Enum value = ezVariantType::TempHashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezTempHashedString;
};

template <>
struct ezVariantTypeDeduction<ezVariantArray>
{
  static constexpr ezVariantType::Enum value = ezVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVariantArray;
};

template <>
struct ezVariantTypeDeduction<ezArrayPtr<ezVariant>>
{
  static constexpr ezVariantType::Enum value = ezVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezVariantArray;
};


template <>
struct ezVariantTypeDeduction<ezVariantDictionary>
{
  static constexpr ezVariantType::Enum value = ezVariantType::VariantDictionary;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

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
  static constexpr ezVariantType::Enum value = ezVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::DirectCast;

  using StorageType = ezTypedPointer;
};

template <typename T>
struct ezVariantTypeDeduction<T*>
{
  static constexpr ezVariantType::Enum value = ezVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::PointerCast;

  using StorageType = ezTypedPointer;
};

template <>
struct ezVariantTypeDeduction<ezTypedObject>
{
  static constexpr ezVariantType::Enum value = ezVariantType::TypedObject;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr ezVariantClass::Enum classification = ezVariantClass::TypedObject;

  using StorageType = ezTypedObject;
};

/// \endcond
