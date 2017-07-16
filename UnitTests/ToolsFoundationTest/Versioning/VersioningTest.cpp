#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/GraphVersioning.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Versioning);
///
class ezFloatStruct_1_2 : public ezGraphPatch
{
public:
  ezFloatStruct_1_2() : ezGraphPatch("ezFloatStruct", 2) {}
  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->ChangeProperty("Double", (double)10.0);
    pNode->RenameProperty("Time", "TimeRenamed");
  }
};
ezFloatStruct_1_2 g_ezFloatStruct_1_2;

///
class ezPODClass_1_2 : public ezGraphPatch
{
public:
  ezPODClass_1_2() : ezGraphPatch("ezPODClass", 2) {}
  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->ChangeProperty("Color", ezColor(0.0f, 1.0f, 0.0f, 0.0f));
  }
};
ezPODClass_1_2 g_ezPODClass_1_2;


ezAbstractObjectNode* SerializeObject(ezAbstractObjectGraph& graph, ezAbstractObjectGraph& typesGraph, const ezRTTI* pRtti, void* pObject)
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
    ezReflectionUtils::GatherDependentTypes(pRtti, types);
    ezToolsReflectionUtils::SerializeTypes(types, typesGraph);
  }
  ezGraphVersioning::GetSingleton()->PatchGraph(&typesGraph);
  ezGraphVersioning::GetSingleton()->PatchGraph(&graph, &typesGraph);
  return pNode;
}

EZ_CREATE_SIMPLE_TEST(Versioning, GraphPatch)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PatchClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezFloatStruct data;
    data.SetDouble(5.0);
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezFloatStruct>(), &data);

    ezAbstractObjectNode::Property* pDouble = pNode->FindProperty("Double");
    EZ_TEST_FLOAT(10.0, pDouble->m_Value.Get<double>(), 0.0);
    ezAbstractObjectNode::Property* pTime = pNode->FindProperty("TimeRenamed");
    EZ_TEST_BOOL(pNode->FindProperty("TimeRenamed") != nullptr);
    EZ_TEST_BOOL(pNode->FindProperty("Time") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PatchBaseClass")
  {
    ezAbstractObjectGraph graph;
    ezAbstractObjectGraph typesGraph;

    ezMathClass data;
    data.SetColor(ezColor(1.0f, 0.0f, 0.0f, 0.0f));
    ezAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, ezGetStaticRTTI<ezMathClass>(), &data);

    ezAbstractObjectNode::Property* pColor = pNode->FindProperty("Color");
    EZ_TEST_BOOL(pColor->m_Value.Get<ezColor>() == ezColor(0.0f, 1.0f, 0.0f, 0.0f));
  }


}
