#include <PCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Serialization);

class TestContext : public ezRttiConverterContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override
  {
    void* pObj = pRtti->GetAllocator()->Allocate<void>();
    RegisterObject(guid, pRtti, pObj);
    return pObj;
  }

  virtual void DeleteObject(const ezUuid& guid) override
  {
    auto object = GetObjectByGUID(guid);
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);

    UnregisterObject(guid);
  }
};

template <typename T>
void TestSerialize(T* pObject)
{
  ezAbstractObjectGraph graph;
  TestContext context;
  ezRttiConverterWriter conv(&graph, &context, true, true);

  const ezRTTI* pRtti = ezGetStaticRTTI<T>();
  ezUuid guid;
  guid.CreateNewUuid();

  context.RegisterObject(guid, pRtti, pObject);
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, pObject, "root");

  EZ_TEST_BOOL(pNode->GetGuid() == guid);
  EZ_TEST_STRING(pNode->GetType(), pRtti->GetTypeName());
  EZ_TEST_INT(pNode->GetProperties().GetCount(), pNode->GetProperties().GetCount());

  {
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezAbstractGraphDdlSerializer::Write(writer, &graph);

    ezStringBuilder sData, sData2;
    sData.SetSubString_ElementCount((const char*)storage.GetData(), storage.GetStorageSize());


    ezRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    EZ_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    EZ_TEST_BOOL(target == *pObject);

    // Overwrite again to test for leaks as existing values have to be removed first by ezRttiConverterReader.
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    EZ_TEST_BOOL(target == *pObject);

    {
      T clone;
      ezReflectionSerializer::Clone(pObject, &clone, pRtti);
      EZ_TEST_BOOL(clone == *pObject);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&clone, pObject, pRtti));
    }

    {
      T* pClone = ezReflectionSerializer::Clone(pObject);
      EZ_TEST_BOOL(*pClone == *pObject);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(pClone, pObject));
      // Overwrite again to test for leaks as existing values have to be removed first by clone.
      ezReflectionSerializer::Clone(pObject, pClone, pRtti);
      EZ_TEST_BOOL(*pClone == *pObject);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(pClone, pObject, pRtti));
      pRtti->GetAllocator()->Deallocate(pClone);
    }

    ezAbstractObjectGraph graph2;
    ezAbstractGraphDdlSerializer::Read(reader, &graph2);

    ezMemoryStreamStorage storage2;
    ezMemoryStreamWriter writer2(&storage2);

    ezAbstractGraphDdlSerializer::Write(writer2, &graph2);
    sData2.SetSubString_ElementCount((const char*)storage2.GetData(), storage2.GetStorageSize());

    EZ_TEST_BOOL(sData == sData2);
  }

  {
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezAbstractGraphBinarySerializer::Write(writer, &graph);

    ezRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    EZ_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    EZ_TEST_BOOL(target == *pObject);

    ezAbstractObjectGraph graph2;
    ezAbstractGraphBinarySerializer::Read(reader, &graph2);

    ezMemoryStreamStorage storage2;
    ezMemoryStreamWriter writer2(&storage2);

    ezAbstractGraphBinarySerializer::Write(writer2, &graph2);

    EZ_TEST_INT(storage.GetStorageSize(), storage2.GetStorageSize());

    if (storage.GetStorageSize() == storage2.GetStorageSize())
    {
      EZ_TEST_BOOL(ezMemoryUtils::ByteCompare<ezUInt8>(storage.GetData(), storage2.GetData(), storage.GetStorageSize()) == 0);
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Serialization, RttiConverter)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PODs")
  {
    ezTestStruct t1;
    t1.m_fFloat1 = 5.0f;
    t1.m_UInt8 = 222;
    t1.m_variant = "A";
    t1.m_Angle = ezAngle::Degree(5);
    t1.m_DataBuffer.PushBack(1);
    t1.m_DataBuffer.PushBack(5);
    t1.m_vVec3I = ezVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      ezTestStruct clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestStruct>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestStruct>()));
      clone.m_variant = "Test";
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestStruct>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EmbededStruct")
  {
    ezTestClass1 t1;
    t1.m_Color = ezColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = ezAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = ezVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      ezTestClass1 clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestClass1>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      clone.m_Struct.m_variant = ezVec3(1, 2, 3);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass1>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enum")
  {
    ezTestEnumStruct t1;
    t1.m_enum = ezExampleEnum::Value2;
    t1.m_enumClass = ezExampleEnum::Value3;
    t1.SetEnum(ezExampleEnum::Value2);
    t1.SetEnumClass(ezExampleEnum::Value3);
    TestSerialize(&t1);

    {
      ezTestEnumStruct clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestEnumStruct>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestEnumStruct>()));
      clone.m_enum = ezExampleEnum::Value3;
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestEnumStruct>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitflags")
  {
    ezTestBitflagsStruct t1;
    t1.m_bitflagsClass.SetValue(0);
    t1.SetBitflagsClass(ezExampleBitflags::Value1 | ezExampleBitflags::Value2);
    TestSerialize(&t1);

    {
      ezTestBitflagsStruct clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestBitflagsStruct>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestBitflagsStruct>()));
      clone.m_bitflagsClass = ezExampleBitflags::Value1;
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestBitflagsStruct>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Derived Class")
  {
    ezTestClass2 t1;
    t1.m_Color = ezColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = ezAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = ezVec3I32(0, 1, 333);
    t1.m_Time = ezTime::Seconds(22.2f);
    t1.m_enumClass = ezExampleEnum::Value3;
    t1.m_bitflagsClass = ezExampleBitflags::Value1 | ezExampleBitflags::Value2;
    t1.m_array.PushBack(40.0f);
    t1.m_array.PushBack(-1.5f);
    t1.m_Variant = ezVec4(1, 2, 3, 4);
    t1.SetText("LALALALA");
    TestSerialize(&t1);

    {
      ezTestClass2 clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestClass2>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      t1.m_array.PushBack(-1.33f);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestClass2>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Arrays")
  {
    ezTestArrays t1;
    t1.m_Hybrid.PushBack(4.5f);
    t1.m_Hybrid.PushBack(2.3f);
    t1.m_HybridChar.PushBack("Test");

    ezTestStruct3 ts;
    ts.m_fFloat1 = 5.0f;
    ts.m_UInt8 = 22;
    t1.m_Dynamic.PushBack(ts);
    t1.m_Dynamic.PushBack(ts);
    t1.m_Deque.PushBack(ezTestArrays());
    TestSerialize(&t1);

    {
      ezTestArrays clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestArrays>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestArrays>()));
      clone.m_Dynamic.PushBack(ezTestStruct3());
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestArrays>()));
      clone.m_Dynamic.PopBack();
      clone.m_Hybrid.PushBack(444.0f);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestArrays>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sets")
  {
    ezTestSets t1;
    t1.m_SetMember.Insert(0);
    t1.m_SetMember.Insert(5);
    t1.m_SetMember.Insert(-33);
    t1.m_SetAccessor.Insert(-0.0f);
    t1.m_SetAccessor.Insert(5.4f);
    t1.m_SetAccessor.Insert(-33.0f);
    t1.m_Deque.PushBack(3);
    t1.m_Deque.PushBack(33);
    t1.m_Array.PushBack("Test");
    t1.m_Array.PushBack("Bla");
    TestSerialize(&t1);

    {
      ezTestSets clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestSets>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestSets>()));
      clone.m_SetMember.Insert(12);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestSets>()));
      clone.m_SetMember.Remove(12);
      clone.m_Array.PushBack("Bla2");
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestSets>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pointer")
  {
    ezTestPtr t1;
    t1.m_sString = "Ttttest";
    t1.m_pArrays = EZ_DEFAULT_NEW(ezTestArrays);
    t1.m_pArraysDirect = EZ_DEFAULT_NEW(ezTestArrays);
    t1.m_ArrayPtr.PushBack(EZ_DEFAULT_NEW(ezTestArrays));
    t1.m_SetPtr.Insert(EZ_DEFAULT_NEW(ezTestSets));
    TestSerialize(&t1);

    {
      ezTestPtr clone;
      ezReflectionSerializer::Clone(&t1, &clone, ezGetStaticRTTI<ezTestPtr>());
      EZ_TEST_BOOL(t1 == clone);
      EZ_TEST_BOOL(ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PushBack(42);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PopBack();
      clone.m_ArrayPtr[0]->m_Hybrid.PushBack(123.0f);
      EZ_TEST_BOOL(!ezReflectionUtils::IsEqual(&t1, &clone, ezGetStaticRTTI<ezTestPtr>()));
    }
  }
}
