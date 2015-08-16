#include <PCH.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Serialization);

class TestContext : public ezRttiConverterContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override
  {
    void* pObj = pRtti->GetAllocator()->Allocate();
    RegisterObject(guid, pRtti, pObj);
    return pObj;
  }

  virtual void DeleteObject(const ezUuid& guid) override
  {
    auto* pObj = GetObjectByGUID(guid);
    pObj->m_pType->GetAllocator()->Deallocate(pObj->m_pObject);

    UnregisterObject(guid);
  }
};

template<typename T>
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

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezMemoryStreamReader reader(&storage);

  ezAbstractGraphJsonSerializer::Write(writer, &graph, ezJSONWriter::WhitespaceMode::All);

  ezStringBuilder sData, sData2;
  sData.SetSubString_ElementCount((const char*)storage.GetData(), storage.GetStorageSize());


  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");
  EZ_TEST_BOOL(pRootNode != nullptr);

  T target;
  convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
  EZ_TEST_BOOL(target == *pObject);

  ezAbstractObjectGraph graph2;
  ezAbstractGraphJsonSerializer::Read(reader, &graph2);

  ezMemoryStreamStorage storage2;
  ezMemoryStreamWriter writer2(&storage2);

  ezAbstractGraphJsonSerializer::Write(writer2, &graph2, ezJSONWriter::WhitespaceMode::All);
  sData2.SetSubString_ElementCount((const char*)storage2.GetData(), storage2.GetStorageSize());

  EZ_TEST_STRING(sData, sData2);
}

EZ_CREATE_SIMPLE_TEST(Serialization, RttiConverter)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PODs")
  {
    ezTestStruct t1;
    TestSerialize(&t1);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EmbededStruct")
  {
    ezTestClass1 t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enum")
  {
    ezTestEnumStruct t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitflags")
  {
    ezTestBitflagsStruct t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Derived Class")
  {
    ezTestClass2 t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Arrays")
  {
    ezTestArrays t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sets")
  {
    ezTestSets t1;
    TestSerialize(&t1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pointer")
  {
    ezTestPtr t1;
    TestSerialize(&t1);
  }
}

