#include <PCH.h>
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

static void WriteGraph(const ezAbstractObjectGraph* pGraph, ezStreamWriter& stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  ezUInt32 iNodes = Nodes.GetCount();
  stream << iNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    stream << node.GetGuid();
    stream << node.GetType();
    stream << node.GetTypeVersion();
    stream << node.GetNodeName();

    const ezHybridArray<ezAbstractObjectNode::Property, 16>& properties = node.GetProperties();
    ezUInt32 iProps = properties.GetCount();
    stream << iProps;
    for (const ezAbstractObjectNode::Property& prop : properties)
    {
      stream << prop.m_szPropertyName;
      stream << prop.m_Value;
    }
  }
}

void ezAbstractGraphBinarySerializer::Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph)
{
  ezUInt32 uiVersion = ezBinarySerializerVersion::CurrentVersion;
  stream << uiVersion;

  WriteGraph(pGraph, stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, stream);
  }
}

static void ReadGraph(ezStreamReader& stream, ezAbstractObjectGraph* pGraph)
{
  ezUInt32 iNodes = 0;
  stream >> iNodes;
  for (ezUInt32 i = 0; i < iNodes; i++)
  {
    ezUuid guid;
    ezUInt32 uiTypeVersion;
    ezStringBuilder sType;
    ezStringBuilder sNodeName;
    stream >> guid;
    stream >> sType;
    stream >> uiTypeVersion;
    stream >> sNodeName;
    ezAbstractObjectNode* pNode = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    ezUInt32 iProps = 0;
    stream >> iProps;
    for (ezUInt32 i = 0; i < iProps; i++)
    {
      ezStringBuilder sPropName;
      ezVariant value;
      stream >> sPropName;
      stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void ezAbstractGraphBinarySerializer::Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  ezUInt32 uiVersion = 0;
  stream >> uiVersion;
  if (uiVersion != ezBinarySerializerVersion::CurrentVersion)
  {
    EZ_REPORT_FAILURE("Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, ezBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    ezGraphVersioning::GetSingleton()->PatchGraph(pGraph);
    if (pTypesGraph)
      ezGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_BinarySerializer);

