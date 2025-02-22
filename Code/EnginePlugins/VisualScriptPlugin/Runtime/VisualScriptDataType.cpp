#include <Core/CorePCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptDataType, 1)
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Invalid),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Bool),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Byte),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Int),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Int64),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Float),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Double),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Color),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Vector3),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Quaternion),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Transform),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Time),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Angle),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::String),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::HashedString),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::GameObject),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Component),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::TypedPointer),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Variant),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Array),
  EZ_ENUM_CONSTANT(ezVisualScriptDataType::Map),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  static constexpr ezVariantType::Enum s_ScriptDataTypeVariantTypes[] = {
    ezVariantType::Invalid,           // Invalid,

    ezVariantType::Bool,              // Bool,
    ezVariantType::UInt8,             // Byte,
    ezVariantType::Int32,             // Int,
    ezVariantType::Int64,             // Int64,
    ezVariantType::Float,             // Float,
    ezVariantType::Double,            // Double,
    ezVariantType::Color,             // Color,
    ezVariantType::Vector3,           // Vector3,
    ezVariantType::Quaternion,        // Quaternion,
    ezVariantType::Transform,         // Transform,
    ezVariantType::Time,              // Time,
    ezVariantType::Angle,             // Angle,
    ezVariantType::String,            // String,
    ezVariantType::HashedString,      // HashedString,
    ezVariantType::TypedObject,       // GameObject,
    ezVariantType::TypedObject,       // Component,
    ezVariantType::TypedPointer,      // TypedPointer,
    ezVariantType::Invalid,           // Variant,
    ezVariantType::VariantArray,      // Array,
    ezVariantType::VariantDictionary, // Map,
    ezVariantType::TypedObject,       // Coroutine,
  };
  static_assert(EZ_ARRAY_SIZE(s_ScriptDataTypeVariantTypes) == (size_t)ezVisualScriptDataType::Count);

  static constexpr ezUInt32 s_ScriptDataTypeSizes[] = {
    0,                                      // Invalid,

    sizeof(bool),                           // Bool,
    sizeof(ezUInt8),                        // Byte,
    sizeof(ezInt32),                        // Int,
    sizeof(ezInt64),                        // Int64,
    sizeof(float),                          // Float,
    sizeof(double),                         // Double,
    sizeof(ezColor),                        // Color,
    sizeof(ezVec3),                         // Vector3,
    sizeof(ezQuat),                         // Quaternion,
    sizeof(ezTransform),                    // Transform,
    sizeof(ezTime),                         // Time,
    sizeof(ezAngle),                        // Angle,
    sizeof(ezString),                       // String,
    sizeof(ezHashedString),                 // HashedString,
    sizeof(ezVisualScriptGameObjectHandle), // GameObject,
    sizeof(ezVisualScriptComponentHandle),  // Component,
    sizeof(ezTypedPointer),                 // TypedPointer,
    sizeof(ezVariant),                      // Variant,
    sizeof(ezVariantArray),                 // Array,
    sizeof(ezVariantDictionary),            // Map,
    sizeof(ezScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(EZ_ARRAY_SIZE(s_ScriptDataTypeSizes) == (size_t)ezVisualScriptDataType::Count);

  static constexpr ezUInt32 s_ScriptDataTypeAlignments[] = {
    0,                                       // Invalid,

    alignof(bool),                           // Bool,
    alignof(ezUInt8),                        // Byte,
    alignof(ezInt32),                        // Int,
    alignof(ezInt64),                        // Int64,
    alignof(float),                          // Float,
    alignof(double),                         // Double,
    alignof(ezColor),                        // Color,
    alignof(ezVec3),                         // Vector3,
    alignof(ezQuat),                         // Quaternion,
    alignof(ezTransform),                    // Transform,
    alignof(ezTime),                         // Time,
    alignof(ezAngle),                        // Angle,
    alignof(ezString),                       // String,
    alignof(ezHashedString),                 // HashedString,
    alignof(ezVisualScriptGameObjectHandle), // GameObject,
    alignof(ezVisualScriptComponentHandle),  // Component,
    alignof(ezTypedPointer),                 // TypedPointer,
    alignof(ezVariant),                      // Variant,
    alignof(ezVariantArray),                 // Array,
    alignof(ezVariantDictionary),            // Map,
    alignof(ezScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(EZ_ARRAY_SIZE(s_ScriptDataTypeAlignments) == (size_t)ezVisualScriptDataType::Count);

  static constexpr const char* s_ScriptDataTypeNames[] = {
    "Invalid",

    "Bool",
    "Byte",
    "Int",
    "Int64",
    "Float",
    "Double",
    "Color",
    "Vector3",
    "Quaternion",
    "Transform",
    "Time",
    "Angle",
    "String",
    "HashedString",
    "GameObject",
    "Component",
    "TypedPointer",
    "Variant",
    "Array",
    "Map",
    "Coroutine",
    "", // Count,
    "Enum",
    "Bitflag",
  };
  static_assert(EZ_ARRAY_SIZE(s_ScriptDataTypeNames) == (size_t)ezVisualScriptDataType::ExtendedCount);
} // namespace

// static
ezVariantType::Enum ezVisualScriptDataType::GetVariantType(Enum dataType)
{
  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_ScriptDataTypeVariantTypes), "Out of bounds access");
  return s_ScriptDataTypeVariantTypes[dataType];
}

// static
ezVisualScriptDataType::Enum ezVisualScriptDataType::FromVariantType(ezVariantType::Enum variantType)
{
  switch (variantType)
  {
    case ezVariantType::Bool:
      return Bool;
    case ezVariantType::Int8:
    case ezVariantType::UInt8:
      return Byte;
    case ezVariantType::Int16:
    case ezVariantType::UInt16:
    case ezVariantType::Int32:
    case ezVariantType::UInt32:
      return Int;
    case ezVariantType::Int64:
    case ezVariantType::UInt64:
      return Int64;
    case ezVariantType::Float:
      return Float;
    case ezVariantType::Double:
      return Double;
    case ezVariantType::Color:
      return Color;
    case ezVariantType::Vector3:
      return Vector3;
    case ezVariantType::Quaternion:
      return Quaternion;
    case ezVariantType::Transform:
      return Transform;
    case ezVariantType::Time:
      return Time;
    case ezVariantType::Angle:
      return Angle;
    case ezVariantType::String:
    case ezVariantType::StringView:
      return String;
    case ezVariantType::HashedString:
    case ezVariantType::TempHashedString:
      return HashedString;
    case ezVariantType::VariantArray:
      return Array;
    case ezVariantType::VariantDictionary:
      return Map;
    default:
      return Invalid;
  }
}

ezProcessingStream::DataType ezVisualScriptDataType::GetStreamDataType(Enum dataType)
{
  // We treat ezColor and ezVec4 as the same in the visual script <=> expression binding
  // so ensure that they have the same size and layout
  static_assert(sizeof(ezColor) == sizeof(ezVec4));
  static_assert(offsetof(ezColor, r) == offsetof(ezVec4, x));
  static_assert(offsetof(ezColor, g) == offsetof(ezVec4, y));
  static_assert(offsetof(ezColor, b) == offsetof(ezVec4, z));
  static_assert(offsetof(ezColor, a) == offsetof(ezVec4, w));

  switch (dataType)
  {
    case Int:
      return ezProcessingStream::DataType::Int;
    case Float:
      return ezProcessingStream::DataType::Float;
    case Vector3:
      return ezProcessingStream::DataType::Float3;
    case Color:
      return ezProcessingStream::DataType::Float4;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return ezProcessingStream::DataType::Float;
}

// static
const ezRTTI* ezVisualScriptDataType::GetRtti(Enum dataType)
{
  // Define table here to prevent issues with static initialization order
  static const ezRTTI* s_Rttis[] = {
    nullptr,                                    // Invalid,

    ezGetStaticRTTI<bool>(),                    // Bool,
    ezGetStaticRTTI<ezUInt8>(),                 // Byte,
    ezGetStaticRTTI<ezInt32>(),                 // Int,
    ezGetStaticRTTI<ezInt64>(),                 // Int64,
    ezGetStaticRTTI<float>(),                   // Float,
    ezGetStaticRTTI<double>(),                  // Double,
    ezGetStaticRTTI<ezColor>(),                 // Color,
    ezGetStaticRTTI<ezVec3>(),                  // Vector3,
    ezGetStaticRTTI<ezQuat>(),                  // Quaternion,
    ezGetStaticRTTI<ezTransform>(),             // Transform,
    ezGetStaticRTTI<ezTime>(),                  // Time,
    ezGetStaticRTTI<ezAngle>(),                 // Angle,
    ezGetStaticRTTI<ezString>(),                // String,
    ezGetStaticRTTI<ezHashedString>(),          // HashedString,
    ezGetStaticRTTI<ezGameObjectHandle>(),      // GameObject,
    ezGetStaticRTTI<ezComponentHandle>(),       // Component,
    nullptr,                                    // TypedPointer,
    ezGetStaticRTTI<ezVariant>(),               // Variant,
    ezGetStaticRTTI<ezVariantArray>(),          // Array,
    ezGetStaticRTTI<ezVariantDictionary>(),     // Map,
    ezGetStaticRTTI<ezScriptCoroutineHandle>(), // Coroutine,
    nullptr,                                    // Count,
    nullptr,                                    // EnumValue,
    nullptr,                                    // BitflagValue,
  };
  static_assert(EZ_ARRAY_SIZE(s_Rttis) == (size_t)ezVisualScriptDataType::ExtendedCount);

  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_Rttis), "Out of bounds access");
  return s_Rttis[dataType];
}

// static
ezVisualScriptDataType::Enum ezVisualScriptDataType::FromRtti(const ezRTTI* pRtti)
{
  Enum res = FromVariantType(pRtti->GetVariantType());
  if (res != Invalid)
    return res;

  if (pRtti->IsDerivedFrom<ezGameObject>() || pRtti == ezGetStaticRTTI<ezGameObjectHandle>())
    return GameObject;

  if (pRtti->IsDerivedFrom<ezComponent>() || pRtti == ezGetStaticRTTI<ezComponentHandle>())
    return Component;

  if (pRtti == ezGetStaticRTTI<ezScriptCoroutineHandle>())
    return Coroutine;

  if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::Class))
    return TypedPointer;

  if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::IsEnum))
    return EnumValue;

  if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::Bitflags))
    return BitflagValue;

  if (pRtti == ezGetStaticRTTI<ezVariant>())
    return Variant;

  return Invalid;
}

// static
ezUInt32 ezVisualScriptDataType::GetStorageSize(Enum dataType)
{
  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_ScriptDataTypeSizes), "Out of bounds access");
  return s_ScriptDataTypeSizes[dataType];
}

// static
ezUInt32 ezVisualScriptDataType::GetStorageAlignment(Enum dataType)
{
  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_ScriptDataTypeAlignments), "Out of bounds access");
  return s_ScriptDataTypeAlignments[dataType];
}

// static
const char* ezVisualScriptDataType::GetName(Enum dataType)
{
  if (dataType == AnyPointer)
  {
    return "Pointer";
  }
  else if (dataType == Any)
  {
    return "Any";
  }

  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_ScriptDataTypeNames), "Out of bounds access");
  return s_ScriptDataTypeNames[dataType];
}

// static
bool ezVisualScriptDataType::CanConvertTo(Enum sourceDataType, Enum targetDataType)
{
  if (sourceDataType == targetDataType ||
      targetDataType == String ||
      targetDataType == HashedString ||
      targetDataType == Variant)
    return true;

  if ((IsNumber(sourceDataType) || (sourceDataType == EnumValue || sourceDataType == BitflagValue)) &&
      (IsNumber(targetDataType) || (targetDataType == EnumValue || targetDataType == BitflagValue)))
    return true;

  return false;
}

//////////////////////////////////////////////////////////////////////////

ezGameObject* ezVisualScriptGameObjectHandle::GetPtr(ezUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (ezWorld* pWorld = ezWorld::GetWorld(m_Handle))
  {
    bool objectExists = pWorld->TryGetObject(m_Handle, m_Ptr);
    EZ_IGNORE_UNUSED(objectExists);
  }

  return m_Ptr;
}

ezComponent* ezVisualScriptComponentHandle::GetPtr(ezUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (ezWorld* pWorld = ezWorld::GetWorld(m_Handle))
  {
    bool componentExists = pWorld->TryGetComponent(m_Handle, m_Ptr);
    EZ_IGNORE_UNUSED(componentExists);
  }

  return m_Ptr;
}


EZ_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Runtime_VisualScriptDataType);
