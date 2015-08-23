#include <Foundation/PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/Implementation/StandardTypes.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Time/Time.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezEnumBase, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBitflagsBase, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectedClass, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

// *********************************************
// ***** Standard POD Types for Properties *****

EZ_BEGIN_STATIC_REFLECTED_TYPE(bool, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(float, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(double, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt8, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt8, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt16, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt16, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt32, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt32, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInt64, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUInt64, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezConstCharPtr, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTime, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColor, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("r", r),
    EZ_MEMBER_PROPERTY("g", g),
    EZ_MEMBER_PROPERTY("b", b),
    EZ_MEMBER_PROPERTY("a", a)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec2, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec3, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVec4, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezQuat, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("v", v),
    EZ_MEMBER_PROPERTY("w", w)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMat3, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMat4, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTransform, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezBasisAxis, 1)
  EZ_ENUM_CONSTANT(ezBasisAxis::PositiveX),
  EZ_ENUM_CONSTANT(ezBasisAxis::PositiveY),
  EZ_ENUM_CONSTANT(ezBasisAxis::PositiveZ),
  EZ_ENUM_CONSTANT(ezBasisAxis::NegativeX),
  EZ_ENUM_CONSTANT(ezBasisAxis::NegativeY),
  EZ_ENUM_CONSTANT(ezBasisAxis::NegativeZ)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezUuid, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVariant, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezString, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezTypeFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezTypeFlags::StandardType, ezTypeFlags::Abstract, ezTypeFlags::IsEnum, ezTypeFlags::Bitflags) 
EZ_END_STATIC_REFLECTED_BITFLAGS();

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPropertyFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezPropertyFlags::StandardType, ezPropertyFlags::ReadOnly, ezPropertyFlags::Pointer, ezPropertyFlags::PointerOwner) 
  EZ_BITFLAGS_CONSTANTS(ezPropertyFlags::IsEnum, ezPropertyFlags::Bitflags, ezPropertyFlags::Constant) 
EZ_END_STATIC_REFLECTED_BITFLAGS();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVariantType, 1)
  EZ_BITFLAGS_CONSTANTS(ezVariantType::Invalid, ezVariantType::Bool, ezVariantType::Int8, ezVariantType::UInt8, ezVariantType::Int16, ezVariantType::UInt16)
  EZ_BITFLAGS_CONSTANTS(ezVariantType::Int32, ezVariantType::UInt32, ezVariantType::Int64, ezVariantType::UInt64, ezVariantType::Float, ezVariantType::Double)
  EZ_BITFLAGS_CONSTANTS(ezVariantType::Color, ezVariantType::Vector2, ezVariantType::Vector3, ezVariantType::Vector4, ezVariantType::Quaternion)
  EZ_BITFLAGS_CONSTANTS(ezVariantType::Matrix3, ezVariantType::Matrix4, ezVariantType::String, ezVariantType::Time, ezVariantType::Uuid)
  EZ_BITFLAGS_CONSTANTS(ezVariantType::VariantArray, ezVariantType::VariantDictionary, ezVariantType::ReflectedPointer, ezVariantType::VoidPointer)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyCategory, 1)
  EZ_BITFLAGS_CONSTANTS(ezPropertyCategory::Constant, ezPropertyCategory::Member, ezPropertyCategory::Function, ezPropertyCategory::Array, ezPropertyCategory::Set, ezPropertyCategory::Map)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);

