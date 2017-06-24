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


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAbstractTestClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAbstractTestStruct, ezNoBase, 1, ezRTTINoAllocator);
EZ_END_STATIC_REFLECTED_TYPE


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct, ezNoBase, 7, ezRTTIDefaultAllocator<ezTestStruct>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Float", m_fFloat1),
    EZ_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3),
    EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
    EZ_MEMBER_PROPERTY("Variant", m_variant),
    EZ_MEMBER_PROPERTY("Angle", m_Angle),
    EZ_MEMBER_PROPERTY("DataBuffer", m_DataBuffer),
    EZ_MEMBER_PROPERTY("vVec3I", m_vVec3I),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct3, ezNoBase, 71, ezRTTIDefaultAllocator<ezTestStruct3>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Float", m_fFloat1),
    EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(),
    EZ_CONSTRUCTOR_PROPERTY(double, ezInt16),
  }
  EZ_END_FUNCTIONS
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass1, 11, ezRTTIDefaultAllocator<ezTestClass1>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SubStruct", m_Struct),
    // EZ_MEMBER_PROPERTY("MyVector", m_MyVector), Intentionally not reflected
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("SubVector", GetVector)
  }
    EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezInt32 ezTestClass2Allocator::m_iAllocs = 0;
ezInt32 ezTestClass2Allocator::m_iDeallocs = 0;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2, 22, ezTestClass2Allocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Text", GetText, SetText),
    EZ_MEMBER_PROPERTY("Time", m_Time),
    EZ_ENUM_MEMBER_PROPERTY("Enum", ezExampleEnum, m_enumClass),
    EZ_BITFLAGS_MEMBER_PROPERTY("Bitflags", ezExampleBitflags, m_bitflagsClass),
    EZ_ARRAY_MEMBER_PROPERTY("Array", m_array),
    EZ_MEMBER_PROPERTY("Variant", m_Variant),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2b, 24, ezRTTIDefaultAllocator<ezTestClass2b>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
    EZ_MEMBER_PROPERTY("SubStruct", m_Struct),
    EZ_MEMBER_PROPERTY("Color", m_Color),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestArrays, 1, ezRTTIDefaultAllocator<ezTestArrays>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
    EZ_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
    EZ_ARRAY_MEMBER_PROPERTY("Deque", m_Deque),
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridRO", m_Hybrid),
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("DynamicRO", m_Dynamic),
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("DequeRO", m_Deque),

    EZ_ARRAY_ACCESSOR_PROPERTY("AcHybrid", GetCount, GetValue, SetValue, Insert, Remove),
    EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridRO", GetCount, GetValue),
    EZ_ARRAY_ACCESSOR_PROPERTY("AcHybridChar", GetCountChar, GetValueChar, SetValueChar, InsertChar, RemoveChar),
    EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridCharRO", GetCountChar, GetValueChar),
    EZ_ARRAY_ACCESSOR_PROPERTY("AcDynamic", GetCountDyn, GetValueDyn, SetValueDyn, InsertDyn, RemoveDyn),
    EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDynamicRO", GetCountDyn, GetValueDyn),
    EZ_ARRAY_ACCESSOR_PROPERTY("AcDeque", GetCountDeq, GetValueDeq, SetValueDeq, InsertDeq, RemoveDeq),
    EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDequeRO", GetCountDeq, GetValueDeq),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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

ezUInt32 ezTestArrays::GetCountChar() const
{
  return m_HybridChar.GetCount();
}
const char* ezTestArrays::GetValueChar(ezUInt32 uiIndex) const
{
  return m_HybridChar[uiIndex];
}
void ezTestArrays::SetValueChar(ezUInt32 uiIndex, const char* value)
{
  m_HybridChar[uiIndex] = value;
}
void ezTestArrays::InsertChar(ezUInt32 uiIndex, const char* value)
{
  m_HybridChar.Insert(value, uiIndex);
}
void ezTestArrays::RemoveChar(ezUInt32 uiIndex)
{
  m_HybridChar.RemoveAt(uiIndex);
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestSets, 1, ezRTTIDefaultAllocator<ezTestSets>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("Set", m_SetMember),
    EZ_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
    EZ_SET_ACCESSOR_PROPERTY("AcSet", GetSet, Insert, Remove),
    EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcSetRO", GetSet),
    EZ_SET_MEMBER_PROPERTY("HashSet", m_HashSetMember),
    EZ_SET_MEMBER_PROPERTY_READ_ONLY("HashSetRO", m_HashSetMember),
    EZ_SET_ACCESSOR_PROPERTY("HashAcSet", GetHashSet, HashInsert, HashRemove),
    EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("HashAcSetRO", GetHashSet),
    EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet", GetPseudoSet, PseudoInsert, PseudoRemove),
    EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSetRO", GetPseudoSet),
    EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet2", GetPseudoSet2, PseudoInsert2, PseudoRemove2),
    EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSet2RO", GetPseudoSet2),
    EZ_SET_ACCESSOR_PROPERTY("AcPseudoSet2b", GetPseudoSet2, PseudoInsert2b, PseudoRemove2b),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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


const ezHashSet<ezInt64>& ezTestSets::GetHashSet() const
{
  return m_HashSetAccessor;
}

void ezTestSets::HashInsert(ezInt64 value)
{
  m_HashSetAccessor.Insert(value);
}

void ezTestSets::HashRemove(ezInt64 value)
{
  m_HashSetAccessor.Remove(value);
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestMaps, 1, ezRTTIDefaultAllocator<ezTestMaps>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MAP_MEMBER_PROPERTY("Map", m_MapMember),
    EZ_MAP_MEMBER_PROPERTY_READ_ONLY("MapRO", m_MapMember),
    EZ_MAP_MEMBER_PROPERTY("HashTable", m_HashTableMember),
    EZ_MAP_MEMBER_PROPERTY_READ_ONLY("HashTableRO", m_HashTableMember),
    EZ_MAP_ACCESSOR_PROPERTY("AcMap", GetContainer, Insert, Remove),
    EZ_MAP_ACCESSOR_PROPERTY_READ_ONLY("AcMapRO", GetContainer),
    EZ_MAP_ACCESSOR_PROPERTY("AcHashTable", GetContainer2, Insert2, Remove2),
    EZ_MAP_ACCESSOR_PROPERTY_READ_ONLY("AcHashTableRO", GetContainer2),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezTestMaps::Insert(const char* szKey, ezInt64 value)
{
  m_MapAccessor.Insert(szKey, value);
}

void ezTestMaps::Remove(const char* szKey)
{
  m_MapAccessor.Remove(szKey);
}


const ezHashTable<ezString, ezString>& ezTestMaps::GetContainer2() const
{
  return m_HashTableAccessor;
}


void ezTestMaps::Insert2(const char* szKey, const ezString& value)
{
  m_HashTableAccessor.Insert(szKey, value);
}


void ezTestMaps::Remove2(const char* szKey)
{
  m_HashTableAccessor.Remove(szKey);
}

const ezMap<ezString, ezInt64>& ezTestMaps::GetContainer() const
{
  return m_MapAccessor;
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestPtr, 1, ezRTTIDefaultAllocator<ezTestPtr>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ConstCharPtr", GetString, SetString),
    EZ_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_MEMBER_PROPERTY("ArraysPtrDirect", m_pArraysDirect)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_ARRAY_MEMBER_PROPERTY("PtrArray", m_ArrayPtr)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_SET_MEMBER_PROPERTY("PtrSet", m_SetPtr)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestEnumStruct, ezNoBase, 1, ezRTTIDefaultAllocator<ezTestEnumStruct>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("m_enum", ezExampleEnum, m_enum),
    EZ_ENUM_MEMBER_PROPERTY("m_enumClass", ezExampleEnum, m_enumClass),
    EZ_ENUM_ACCESSOR_PROPERTY("m_enum2", ezExampleEnum, GetEnum, SetEnum),
    EZ_ENUM_ACCESSOR_PROPERTY("m_enumClass2", ezExampleEnum,  GetEnumClass, SetEnumClass),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestBitflagsStruct, ezNoBase, 1, ezRTTIDefaultAllocator<ezTestBitflagsStruct>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_MEMBER_PROPERTY("m_bitflagsClass", ezExampleBitflags, m_bitflagsClass),
    EZ_BITFLAGS_ACCESSOR_PROPERTY("m_bitflagsClass2", ezExampleBitflags, GetBitflagsClass, SetBitflagsClass),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

