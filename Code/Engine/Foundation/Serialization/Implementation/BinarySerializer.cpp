#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

enum ezBinarySerializerVersion : ezUInt32
{
  InvalidVersion = 0,
  Version1,
  // << insert new versions here >>

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

static void WriteGraph(const ezAbstractObjectGraph* pGraph, ezStreamWriter& inout_stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  ezUInt32 uiNodes = Nodes.GetCount();
  inout_stream << uiNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    inout_stream << node.GetGuid();
    inout_stream << node.GetType();
    inout_stream << node.GetTypeVersion();
    inout_stream << node.GetNodeName();

    const ezHybridArray<ezAbstractObjectNode::Property, 16>& properties = node.GetProperties();
    ezUInt32 uiProps = properties.GetCount();
    inout_stream << uiProps;
    for (const ezAbstractObjectNode::Property& prop : properties)
    {
      inout_stream << prop.m_sPropertyName;
      inout_stream << prop.m_Value;
    }
  }
}

void ezAbstractGraphBinarySerializer::Write(ezStreamWriter& inout_stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph)
{
  ezUInt32 uiVersion = ezBinarySerializerVersion::CurrentVersion;
  inout_stream << uiVersion;

  WriteGraph(pGraph, inout_stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, inout_stream);
  }
}

static void ReadGraph(ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph)
{
  ezUInt32 uiNodes = 0;
  inout_stream >> uiNodes;
  for (ezUInt32 uiNodeIdx = 0; uiNodeIdx < uiNodes; uiNodeIdx++)
  {
    ezUuid guid;
    ezUInt32 uiTypeVersion;
    ezStringBuilder sType;
    ezStringBuilder sNodeName;
    inout_stream >> guid;
    inout_stream >> sType;
    inout_stream >> uiTypeVersion;
    inout_stream >> sNodeName;
    ezAbstractObjectNode* pNode = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    ezUInt32 uiProps = 0;
    inout_stream >> uiProps;
    for (ezUInt32 propIdx = 0; propIdx < uiProps; ++propIdx)
    {
      ezStringBuilder sPropName;
      ezVariant value;
      inout_stream >> sPropName;
      inout_stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void ezAbstractGraphBinarySerializer::Read(
  ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  ezUInt32 uiVersion = 0;
  inout_stream >> uiVersion;
  if (uiVersion != ezBinarySerializerVersion::CurrentVersion)
  {
    EZ_REPORT_FAILURE(
      "Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, ezBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(inout_stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(inout_stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    if (pTypesGraph)
      ezGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
    ezGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTypesGraph);
  }
}


