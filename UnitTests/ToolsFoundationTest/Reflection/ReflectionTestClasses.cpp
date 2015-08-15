#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezIntegerStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("Int8", GetInt8, SetInt8),
  EZ_ACCESSOR_PROPERTY("UInt8", GetUInt8, SetUInt8),
  EZ_MEMBER_PROPERTY("Int16", m_iInt16),
  EZ_MEMBER_PROPERTY("UInt16", m_iUInt16),
  EZ_ACCESSOR_PROPERTY("Int32", GetInt32, SetInt32),
  EZ_ACCESSOR_PROPERTY("UInt32", GetUInt32, SetUInt32),
  EZ_MEMBER_PROPERTY("Int64", m_iInt64),
  EZ_MEMBER_PROPERTY("UInt64", m_iUInt64)
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFloatStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("Float", GetFloat, SetFloat),
  EZ_ACCESSOR_PROPERTY("Double", GetDouble, SetDouble),
  EZ_ACCESSOR_PROPERTY("Time", GetTime, SetTime)
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPODClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezPODClass>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("Integer", m_IntegerStruct),
  EZ_MEMBER_PROPERTY("Float", m_FloatStruct),
  EZ_ACCESSOR_PROPERTY("Bool", GetBool, SetBool),
  EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
  EZ_ACCESSOR_PROPERTY("String", GetString, SetString)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMathClass, ezPODClass, 1, ezRTTIDefaultAllocator<ezMathClass>);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("Vec2", GetVec2, SetVec2),
  EZ_ACCESSOR_PROPERTY("Vec3", GetVec3, SetVec3),
  EZ_ACCESSOR_PROPERTY("Vec4", GetVec4, SetVec4),
  EZ_ACCESSOR_PROPERTY("Quat", GetQuat, SetQuat),
  EZ_ACCESSOR_PROPERTY("Mat3", GetMat3, SetMat3),
  EZ_ACCESSOR_PROPERTY("Mat4", GetMat4, SetMat4)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezExampleEnum, 1)
  EZ_ENUM_CONSTANTS(ezExampleEnum::Value1, ezExampleEnum::Value2) 
  EZ_ENUM_CONSTANT(ezExampleEnum::Value3),
EZ_END_STATIC_REFLECTED_ENUM();


EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezExampleBitflags, 1)
  EZ_BITFLAGS_CONSTANTS(ezExampleBitflags::Value1, ezExampleBitflags::Value2) 
  EZ_BITFLAGS_CONSTANT(ezExampleBitflags::Value3),
EZ_END_STATIC_REFLECTED_BITFLAGS();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEnumerationsClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezEnumerationsClass>);
EZ_BEGIN_PROPERTIES
  EZ_ENUM_ACCESSOR_PROPERTY("Enum", ezExampleEnum, GetEnum, SetEnum),
  EZ_BITFLAGS_ACCESSOR_PROPERTY("Bitflags", ezExampleBitflags, GetBitflags, SetBitflags),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_TYPE(InnerStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("IP1", m_fP1),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(OuterClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<OuterClass>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("Inner", m_Inner1),
  EZ_MEMBER_PROPERTY("OP1", m_fP1),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectTest, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezObjectTest>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("MemberClass", m_MemberClass),
EZ_ARRAY_MEMBER_PROPERTY("StandardTypeArray", m_StandardTypeArray),
EZ_ARRAY_MEMBER_PROPERTY("ClassArray", m_ClassArray),
EZ_ARRAY_MEMBER_PROPERTY("ClassPtrArray", m_ClassPtrArray)->AddFlags(ezPropertyFlags::PointerOwner),
EZ_SET_ACCESSOR_PROPERTY("StandardTypeSet", GetStandardTypeSet, StandardTypeSetInsert, StandardTypeSetRemove),
EZ_SET_MEMBER_PROPERTY("SubObjectSet", m_SubObjectSet)->AddFlags(ezPropertyFlags::PointerOwner),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezArrayPtr<const ezString> ezObjectTest::GetStandardTypeSet() const
{
  return m_StandardTypeSet;
}

void ezObjectTest::StandardTypeSetInsert(const ezString& value)
{
  if (!m_StandardTypeSet.Contains(value))
    m_StandardTypeSet.PushBack(value);
}

void ezObjectTest::StandardTypeSetRemove(const ezString& value)
{
  m_StandardTypeSet.Remove(value);
}
