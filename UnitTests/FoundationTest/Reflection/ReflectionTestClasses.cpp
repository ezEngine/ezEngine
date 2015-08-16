#include <PCH.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezExampleEnum, 1)
  EZ_ENUM_CONSTANTS(ezExampleEnum::Value1, ezExampleEnum::Value2) 
  EZ_ENUM_CONSTANT(ezExampleEnum::Value3),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezExampleBitflags, 1)
  EZ_BITFLAGS_CONSTANTS(ezExampleBitflags::Value1, ezExampleBitflags::Value2) 
  EZ_BITFLAGS_CONSTANT(ezExampleBitflags::Value3),
EZ_END_STATIC_REFLECTED_BITFLAGS();


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAbstractTestClass, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAbstractTestStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct, ezNoBase, 7, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("Float", m_fFloat1),
  EZ_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3),
  EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
  EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
  EZ_MEMBER_PROPERTY("Variant", m_variant),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct3, ezNoBase, 71, ezRTTIDefaultAllocator<ezTestStruct3>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("Float", m_fFloat1),
  EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
  EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass1, ezReflectedClass, 11, ezRTTIDefaultAllocator<ezTestClass1>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("Sub Struct", m_Struct),
  EZ_MEMBER_PROPERTY("Color", m_Color),
  EZ_ACCESSOR_PROPERTY_READ_ONLY("Sub Vector", GetVector)
  EZ_END_PROPERTIES
  EZ_END_DYNAMIC_REFLECTED_TYPE();

ezInt32 ezTestClass2Allocator::m_iAllocs = 0;
ezInt32 ezTestClass2Allocator::m_iDeallocs = 0;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2, ezTestClass1, 22, ezTestClass2Allocator);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("Text", GetText, SetText),
  EZ_MEMBER_PROPERTY("Time", m_Time),
  EZ_ENUM_MEMBER_PROPERTY("Enum", ezExampleEnum, m_enumClass),
  EZ_BITFLAGS_MEMBER_PROPERTY("Bitflags", ezExampleBitflags, m_bitflagsClass),
  EZ_ARRAY_MEMBER_PROPERTY("Array", m_array),
  EZ_MEMBER_PROPERTY("Variant", m_Variant),
  EZ_END_PROPERTIES
  EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2b, ezReflectedClass, 24, ezRTTIDefaultAllocator<ezTestClass2b>);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
  EZ_MEMBER_PROPERTY("Sub Struct", m_Struct),
  EZ_MEMBER_PROPERTY("Color", m_Color),
  EZ_END_PROPERTIES
  EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestArrays, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezTestArrays>);
EZ_BEGIN_PROPERTIES
  EZ_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
  EZ_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
  EZ_ARRAY_MEMBER_PROPERTY("Deque", m_Deque),
  EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridRO", m_Hybrid),
  EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("DynamicRO", m_Dynamic),
  EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("DequeRO", m_Deque),

  EZ_ARRAY_ACCESSOR_PROPERTY("AcHybrid", GetCount, GetValue, SetValue, Insert, Remove),
  EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridRO", GetCount, GetValue),
  EZ_ARRAY_ACCESSOR_PROPERTY("AcDynamic", GetCountDyn, GetValueDyn, SetValueDyn, InsertDyn, RemoveDyn),
  EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDynamicRO", GetCountDyn, GetValueDyn),
  EZ_ARRAY_ACCESSOR_PROPERTY("AcDeque", GetCountDeq, GetValueDeq, SetValueDeq, InsertDeq, RemoveDeq),
  EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDequeRO", GetCountDeq, GetValueDeq),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt32 ezTestArrays::GetCount() const
{
  return m_Hybrid.GetCount();
}
double ezTestArrays::GetValue(ezUInt32 uiIndex) const
{
  return m_Hybrid[uiIndex];
}
void ezTestArrays::SetValue(ezUInt32 uiIndex, double value)
{
  m_Hybrid[uiIndex] = value;
}
void ezTestArrays::Insert(ezUInt32 uiIndex, double value)
{
  m_Hybrid.Insert(value, uiIndex);
}
void ezTestArrays::Remove(ezUInt32 uiIndex)
{
  m_Hybrid.RemoveAt(uiIndex);
}

ezUInt32 ezTestArrays::GetCountDyn() const
{
  return m_Dynamic.GetCount();
}
const ezTestStruct3& ezTestArrays::GetValueDyn(ezUInt32 uiIndex) const
{
  return m_Dynamic[uiIndex];
}
void ezTestArrays::SetValueDyn(ezUInt32 uiIndex, const ezTestStruct3& value)
{
  m_Dynamic[uiIndex] = value;
}
void ezTestArrays::InsertDyn(ezUInt32 uiIndex, const ezTestStruct3& value)
{
  m_Dynamic.Insert(value, uiIndex);
}
void ezTestArrays::RemoveDyn(ezUInt32 uiIndex)
{
  m_Dynamic.RemoveAt(uiIndex);
}

ezUInt32 ezTestArrays::GetCountDeq() const
{
  return m_Deque.GetCount();
}
const ezTestArrays& ezTestArrays::GetValueDeq(ezUInt32 uiIndex) const
{
  return m_Deque[uiIndex];
}
void ezTestArrays::SetValueDeq(ezUInt32 uiIndex, const ezTestArrays& value)
{
  m_Deque[uiIndex] = value;
}
void ezTestArrays::InsertDeq(ezUInt32 uiIndex, const ezTestArrays& value)
{
  m_Deque.Insert(value, uiIndex);
}
void ezTestArrays::RemoveDeq(ezUInt32 uiIndex)
{
  m_Deque.RemoveAt(uiIndex);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestSets, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezTestSets>);
EZ_BEGIN_PROPERTIES
  EZ_SET_MEMBER_PROPERTY("Set", m_SetMember),
  EZ_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
  EZ_SET_ACCESSOR_PROPERTY("AcSet", GetSet, Insert, Remove),
  EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcSetRO", GetSet),
  EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet", GetPseudoSet, PseudoInsert, PseudoRemove),
  EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSetRO", GetPseudoSet),
  EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet2", GetPseudoSet2, PseudoInsert2, PseudoRemove2),
  EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSet2RO", GetPseudoSet2),
  EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet2b", GetPseudoSet2, PseudoInsert2b, PseudoRemove2b),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

const ezSet<double>& ezTestSets::GetSet() const
{
  return m_SetAccessor;
}

void ezTestSets::Insert(double value)
{
  m_SetAccessor.Insert(value);
}

void ezTestSets::Remove(double value)
{
  m_SetAccessor.Remove(value);
}


const ezDeque<int>& ezTestSets::GetPseudoSet() const
{
  return m_Deque;
}

void ezTestSets::PseudoInsert(int value)
{
  if (!m_Deque.Contains(value))
    m_Deque.PushBack(value);
}

void ezTestSets::PseudoRemove(int value)
{
  m_Deque.Remove(value);
}


ezArrayPtr<const ezString> ezTestSets::GetPseudoSet2() const
{
  return m_Array;
}

void ezTestSets::PseudoInsert2(const ezString& value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void ezTestSets::PseudoRemove2(const ezString& value)
{
  m_Array.Remove(value);
}

void ezTestSets::PseudoInsert2b(const char* value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void ezTestSets::PseudoRemove2b(const char* value)
{
  m_Array.Remove(value);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestPtr, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezTestPtr>);
EZ_BEGIN_PROPERTIES
  EZ_ACCESSOR_PROPERTY("ConstCharPtr", GetString, SetString),
  EZ_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(ezPropertyFlags::PointerOwner),
  EZ_MEMBER_PROPERTY("ArraysPtrDirect", m_pArraysDirect)->AddFlags(ezPropertyFlags::PointerOwner),
  EZ_ARRAY_MEMBER_PROPERTY("PtrArray", m_ArrayPtr)->AddFlags(ezPropertyFlags::PointerOwner),
  EZ_SET_MEMBER_PROPERTY("PtrSet", m_SetPtr)->AddFlags(ezPropertyFlags::PointerOwner),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestEnumStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_ENUM_MEMBER_PROPERTY("m_enum", ezExampleEnum, m_enum),
  EZ_ENUM_MEMBER_PROPERTY("m_enumClass", ezExampleEnum, m_enumClass),
  EZ_ENUM_ACCESSOR_PROPERTY("m_enum2", ezExampleEnum, GetEnum, SetEnum),
  EZ_ENUM_ACCESSOR_PROPERTY("m_enumClass2", ezExampleEnum,  GetEnumClass, SetEnumClass),
  EZ_END_PROPERTIES
  EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestBitflagsStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
  EZ_BITFLAGS_MEMBER_PROPERTY("m_bitflagsClass", ezExampleBitflags, m_bitflagsClass),
  EZ_BITFLAGS_ACCESSOR_PROPERTY("m_bitflagsClass2", ezExampleBitflags, GetBitflagsClass, SetBitflagsClass),
  EZ_END_PROPERTIES
  EZ_END_STATIC_REFLECTED_TYPE();

