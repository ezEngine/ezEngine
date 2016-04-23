#include <Foundation/PCH.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/BinarySerializer.h>

////////////////////////////////////////////////////////////////////////
// ezReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void ezReflectionSerializer::WriteObjectToJSON(ezStreamWriter& stream, const ezRTTI* pRtti, const void* pObject, ezJSONWriter::WhitespaceMode WhitespaceMode)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, false, true);

  ezUuid guid;
  guid.CreateNewUuid();

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  ezAbstractGraphJsonSerializer::Write(stream, &graph, ezJSONWriter::WhitespaceMode::LessIndentation);
}

void ezReflectionSerializer::WriteObjectToBinary(ezStreamWriter& stream, const ezRTTI* pRtti, const void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, false, true);

  ezUuid guid;
  guid.CreateNewUuid();

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  ezAbstractGraphBinarySerializer::Write(stream, &graph);
}

void* ezReflectionSerializer::ReadObjectFromJSON(ezStreamReader& stream, const ezRTTI*& pRtti)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  pRtti = ezRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, pRtti, pTarget);

  return pTarget;
}

void* ezReflectionSerializer::ReadObjectFromBinary(ezStreamReader& stream, const ezRTTI*& pRtti)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphBinarySerializer::Read(stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  pRtti = ezRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, pRtti, pTarget);

  return pTarget;
}

void ezReflectionSerializer::ReadObjectPropertiesFromJSON(ezStreamReader& stream, const ezRTTI& rtti, void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}

void ezReflectionSerializer::ReadObjectPropertiesFromBinary(ezStreamReader& stream, const ezRTTI& rtti, void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphBinarySerializer::Read(stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_ReflectionSerializer);

