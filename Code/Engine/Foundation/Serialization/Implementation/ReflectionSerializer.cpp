#include <Foundation/PCH.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>


////////////////////////////////////////////////////////////////////////
// ezReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void ezReflectionSerializer::WriteObjectToJSON(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pObject, ezJSONWriter::WhitespaceMode WhitespaceMode)
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

void* ezReflectionSerializer::ReadObjectFromJSON(ezStreamReaderBase& stream, const ezRTTI*& pRtti)
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

void ezReflectionSerializer::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, const ezRTTI& rtti, void* pObject)
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;

  ezAbstractGraphJsonSerializer::Read(stream, &graph);

  ezRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  EZ_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}