#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/GraphVersioning.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Versioning);

struct ezPatchTestBase
{
public:
  ezPatchTestBase()
  {
    m_string = "Base";
    m_string2 = "";
  }

  ezString m_string;
  ezString m_string2;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezPatchTestBase);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPatchTestBase, ezNoBase, 1, ezRTTIDefaultAllocator<ezPatchTestBase>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("String", m_string),
    EZ_MEMBER_PROPERTY("String2", m_string2),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

struct ezPatchTest : public ezPatchTestBase
{
public:
  ezPatchTest()
  {
    m_iInt32 = 1;
  }

  ezInt32 m_iInt32;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezPatchTest);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPatchTest, ezPatchTestBase, 1, ezRTTIDefaultAllocator<ezPatchTest>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Int", m_iInt32),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;


namespace
{
  /// Patch class
  class ezPatchTestP : public ezGraphPatch
  {
  public:
    ezPatchTestP() : ezGraphPatch("ezPatchTestP", 2) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      pNode->RenameProperty("Int", "IntRenamed");
      pNode->ChangeProperty("IntRenamed", 2);
    }
  };
  ezPatchTestP g_ezPatchTestP;

  /// Patch base class
  class ezPatchTestBaseBP : public ezGraphPatch
  {
  public:
    ezPatchTestBaseBP() : ezGraphPatch("ezPatchTestBaseBP", 2) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String", "BaseClassPatched");
    }
  };
  ezPatchTestBaseBP g_ezPatchTestBaseBP;

  /// Rename class
  class ezPatchTestRN : public ezGraphPatch
  {
  public:
    ezPatchTestRN() : ezGraphPatch("ezPatchTestRN", 2) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      context.RenameClass("ezPatchTestRN2");
      pNode->ChangeProperty("String", "RenameExecuted");
    }
  };
  ezPatchTestRN g_ezPatchTestRN;

  /// Patch renamed class to v3
  class ezPatchTestRN2 : public ezGraphPatch
  {
  public:
    ezPatchTestRN2() : ezGraphPatch("ezPatchTestRN2", 3) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String2", "Patched");
    }
  };
  ezPatchTestRN2 g_ezPatchTestRN2;

  /// Change base class
  class ezPatchTestCB : public ezGraphPatch
  {
  public:
    ezPatchTestCB() : ezGraphPatch("ezPatchTestCB", 2) {}
    virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
    {
      ezVersionKey bases[] = { { "ezPatchTestBaseBP", 1 } };
      context.ChangeBaseClass(bases);
      pNode->ChangeProperty("String2", "ChangedBase");
    }
  };
  ezPatchTestCB g_ezPatchTestCB;

  void ReplaceTypeName(ezAbstractObjectGraph& graph, ezAbstractObjectGraph& typesGraph, const char* szOldName, const char* szNewName)
  {
    for (auto* pNode : graph.GetAllNodes())
    {
      if (ezStringUtils::IsEqual(szOldName, pNode->GetType()))
        pNode->SetType(szNewName);
    }

    for (auto* pNode : typesGraph.GetAllNodes())
    {
      if (ezStringUtils::IsEqual("ezReflectedTypeDescriptor", pNode->GetType()))
      {
        if (auto* pProp = pNode->FindProperty("TypeName"))
        {
          if (ezStringUtils::IsEqual(szOldName, pProp->m_Value.Get<ezString>()))
            pProp->m_Value = szNewName;
        }
        if (auto* pProp = pNode->FindProperty("ParentTypeName"))
        {
          if (ezStringUtils::IsEqual(szOldName, pProp->m_Value.Get<ezString>()))
            pProp->m_Value = szNewName;
        }
      }
    }
  }

  ezAbstractObjectNode* SerializeObject(ezAbstractObjectGraph& graph, ezAbstractObjectGraph& typesGraph,
    const ezRTTI* pRtti, void* pObject)
  {
    ezAbstractObjectNode* pNode = nullptr;
    {
      // Object
      ezRttiConverterContext context;
      ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
      context.RegisterObject(ezUuid::StableUuidForString(pRtti->GetTypeName()), pRtti, pObject);
      pNode = rttiConverter.AddObjectToGraph(pRtti, pObject, "ROOT");
    }
    {
      // Types
      ezSet<const ezRTTI*> types;
      types.Insert(pRtti);
      ezReflectionUtils::GatherDependentTypes(pRtti, types);
      ezToolsReflectionUtils::SerializeTypes(types, typesGraph);
    }
    return pNode;
  }

  void PatchGraph(ezAbstractObjectGraph& graph, ezAbstractObjectGraph& typesGraph)
  {
    ezGraphVersioning::GetSingleton()->PatchGraph(&typesGraph);
    ezGraphVersioning::GetSingleton()->PatchGraph(&graph, &typesGraph);
  }
}

EZ_CREATE_SIMPLE_TEST(Versioning, GraphPatch)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PatchClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezPatchTest data;
    data.m_iInt32 = 5;
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "ezPatchTest", "ezPatchTestP");
    PatchGraph(graph, typesGraph);

    ezAbstractObjectNode::Property* pInt = pNode->FindProperty("IntRenamed");
    EZ_TEST_INT(2, pInt->m_Value.Get<ezInt32>());
    EZ_TEST_BOOL(pNode->FindProperty("Int") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PatchBaseClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezPatchTest data;
    data.m_string = "Unpatched";
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "ezPatchTestBase", "ezPatchTestBaseBP");
    PatchGraph(graph, typesGraph);

    ezAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    EZ_TEST_STRING(pString->m_Value.Get<ezString>(), "BaseClassPatched");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RenameClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezPatchTest data;
    data.m_string = "NotRenamed";
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "ezPatchTest", "ezPatchTestRN");
    PatchGraph(graph, typesGraph);

    ezAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    EZ_TEST_BOOL(pString->m_Value.Get<ezString>() == "RenameExecuted");
    EZ_TEST_STRING(pNode->GetType(), "ezPatchTestRN2");
    EZ_TEST_INT(pNode->GetTypeVersion(), 3);
    ezAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    EZ_TEST_BOOL(pString2->m_Value.Get<ezString>() == "Patched");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeBaseClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezPatchTest data;
    data.m_string = "NotPatched";
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "ezPatchTest", "ezPatchTestCB");
    PatchGraph(graph, typesGraph);

    ezAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    EZ_TEST_STRING(pString->m_Value.Get<ezString>(), "BaseClassPatched");
    EZ_TEST_INT(pNode->GetTypeVersion(), 2);
    ezAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    EZ_TEST_STRING(pString2->m_Value.Get<ezString>(), "ChangedBase");
  }
}
