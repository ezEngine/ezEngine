#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezEnumBase, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBitflagsBase, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectedClass, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// *********************************************
// ***** Standard POD Types for Properties *****

EZ_BEGIN_STATIC_REFLECTED_TYPE(bool, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(float, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(double, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt8, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt8, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt16, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt16, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt32, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt32, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt64, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt64, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezConstCharPtr, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTime, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromNanoseconds, In, "Nanoseconds")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromMicroseconds, In, "Microseconds")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromMilliseconds, In, "Milliseconds")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromSeconds, In, "Seconds")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromMinutes, In, "Minutes")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromHours, In, "Hours")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeZero)->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(AsFloatInSeconds),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColor, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("r", r),
    EZ_MEMBER_PROPERTY("g", g),
    EZ_MEMBER_PROPERTY("b", b),
    EZ_MEMBER_PROPERTY("a", a),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(float, float, float),
    EZ_CONSTRUCTOR_PROPERTY(float, float, float, float),
    EZ_CONSTRUCTOR_PROPERTY(ezColorLinearUB),
    EZ_CONSTRUCTOR_PROPERTY(ezColorGammaUB),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeRGBA, In, "R", In, "G", In, "B", In, "A")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeHSV, In, "Hue", In, "Saturation", In, "Value")->AddFlags(ezPropertyFlags::Const),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorBaseUB, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("r", r),
    EZ_MEMBER_PROPERTY("g", g),
    EZ_MEMBER_PROPERTY("b", b),
    EZ_MEMBER_PROPERTY("a", a),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8, ezUInt8),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorGammaUB, ezColorBaseUB, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8, ezUInt8),
    EZ_CONSTRUCTOR_PROPERTY(const ezColor&),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorLinearUB, ezColorBaseUB, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt8, ezUInt8, ezUInt8, ezUInt8),
    EZ_CONSTRUCTOR_PROPERTY(const ezColor&),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec2, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(float),
    EZ_CONSTRUCTOR_PROPERTY(float, float),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec3, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(float),
    EZ_CONSTRUCTOR_PROPERTY(float, float, float),
    EZ_SCRIPT_FUNCTION_PROPERTY(Make, In, "X", In, "Y", In, "Z")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetLength<float>),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetLengthSquared),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetNormalized<float>),
    EZ_SCRIPT_FUNCTION_PROPERTY(Dot, In, "v"),
    EZ_SCRIPT_FUNCTION_PROPERTY(CrossRH, In, "v"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec4, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(float),
    EZ_CONSTRUCTOR_PROPERTY(float, float, float, float),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec2I32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezInt32, ezInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec3I32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezInt32, ezInt32, ezInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec4I32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezInt32, ezInt32, ezInt32, ezInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec2U32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32, ezUInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec3U32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32, ezUInt32, ezUInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec4U32, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32),
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32, ezUInt32, ezUInt32, ezUInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezQuat, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(float, float, float, float),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromAxisAndAngle, In, "Axis", In, "Angle")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeShortestRotation, In, "DirFrom", In, "DirTo")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeSlerp, In, "From", In, "To", In, "Lerp")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetInverse),
    EZ_SCRIPT_FUNCTION_PROPERTY(Rotate, In, "v"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMat3, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMat4, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTransform, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Position", m_vPosition),
    EZ_MEMBER_PROPERTY("Rotation", m_qRotation),
    EZ_MEMBER_PROPERTY("Scale", m_vScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezVec3, ezQuat),
    EZ_CONSTRUCTOR_PROPERTY(ezVec3, ezQuat, ezVec3),
    EZ_SCRIPT_FUNCTION_PROPERTY(Make, In, "Position", In, "Rotation", In, "Scale")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeLocalTransform, In, "Parent", In, "GlobalChild")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeGlobalTransform, In, "Parent", In, "LocalChild")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(TransformPosition, In, "Position"),
    EZ_SCRIPT_FUNCTION_PROPERTY(TransformDirection, In, "Direction"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezBasisAxis, 1)
EZ_ENUM_CONSTANT(ezBasisAxis::PositiveX),
EZ_ENUM_CONSTANT(ezBasisAxis::PositiveY),
EZ_ENUM_CONSTANT(ezBasisAxis::PositiveZ),
EZ_ENUM_CONSTANT(ezBasisAxis::NegativeX),
EZ_ENUM_CONSTANT(ezBasisAxis::NegativeY),
EZ_ENUM_CONSTANT(ezBasisAxis::NegativeZ),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUuid, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVariant, ezNoBase, 3, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVariantArray, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVariantDictionary, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezString, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezStringBuilder, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUntrackedString, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezStringView, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezDataBuffer, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezHashedString, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTempHashedString, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAngle, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromDegree, In, "Degree")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(MakeFromRadian, In, "Radian")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetNormalizedRange),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFloatInterval, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Start", m_StartValue),
    EZ_MEMBER_PROPERTY("End", m_EndValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezIntInterval, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Start", m_StartValue),
    EZ_MEMBER_PROPERTY("End", m_EndValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezTypeFlags, 1)
EZ_BITFLAGS_CONSTANTS(ezTypeFlags::StandardType, ezTypeFlags::IsEnum, ezTypeFlags::Bitflags, ezTypeFlags::Class, ezTypeFlags::Abstract, ezTypeFlags::Phantom, ezTypeFlags::Minimal)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPropertyFlags, 1)
EZ_BITFLAGS_CONSTANTS(ezPropertyFlags::StandardType, ezPropertyFlags::IsEnum, ezPropertyFlags::Bitflags, ezPropertyFlags::Class)
EZ_BITFLAGS_CONSTANTS(ezPropertyFlags::Const, ezPropertyFlags::Reference, ezPropertyFlags::Pointer)
EZ_BITFLAGS_CONSTANTS(ezPropertyFlags::PointerOwner, ezPropertyFlags::ReadOnly, ezPropertyFlags::Hidden, ezPropertyFlags::Phantom)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFunctionType, 1)
EZ_ENUM_CONSTANTS(ezFunctionType::Member, ezFunctionType::StaticMember, ezFunctionType::Constructor)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVariantType, 1)
EZ_ENUM_CONSTANTS(ezVariantType::Invalid, ezVariantType::Bool, ezVariantType::Int8, ezVariantType::UInt8, ezVariantType::Int16, ezVariantType::UInt16)
EZ_ENUM_CONSTANTS(ezVariantType::Int32, ezVariantType::UInt32, ezVariantType::Int64, ezVariantType::UInt64, ezVariantType::Float, ezVariantType::Double)
EZ_ENUM_CONSTANTS(ezVariantType::Color, ezVariantType::Vector2, ezVariantType::Vector3, ezVariantType::Vector4)
EZ_ENUM_CONSTANTS(ezVariantType::Vector2I, ezVariantType::Vector3I, ezVariantType::Vector4I, ezVariantType::Vector2U, ezVariantType::Vector3U, ezVariantType::Vector4U)
EZ_ENUM_CONSTANTS(ezVariantType::Quaternion, ezVariantType::Matrix3, ezVariantType::Matrix4, ezVariantType::Transform)
EZ_ENUM_CONSTANTS(ezVariantType::String, ezVariantType::StringView, ezVariantType::DataBuffer, ezVariantType::Time, ezVariantType::Uuid, ezVariantType::Angle, ezVariantType::ColorGamma)
EZ_ENUM_CONSTANTS(ezVariantType::HashedString, ezVariantType::TempHashedString)
EZ_ENUM_CONSTANTS(ezVariantType::VariantArray, ezVariantType::VariantDictionary, ezVariantType::TypedPointer, ezVariantType::TypedObject)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyCategory, 1)
EZ_ENUM_CONSTANTS(ezPropertyCategory::Constant, ezPropertyCategory::Member, ezPropertyCategory::Function, ezPropertyCategory::Array, ezPropertyCategory::Set, ezPropertyCategory::Map)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);
