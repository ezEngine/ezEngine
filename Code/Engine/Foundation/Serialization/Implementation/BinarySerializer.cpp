#include <Foundation/PCH.h>
#include <Foundation/Serialization/BinarySerializer.h>


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
    ezStringBuilder sType;
    ezStringBuilder sNodeName;
    stream >> guid;
    stream >> sType;
    stream >> sNodeName;
    ezAbstractObjectNode* pNode = pGraph->AddNode(guid, sType, sNodeName);
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

void ezAbstractGraphBinarySerializer::Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph)
{
  ReadGraph(stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(stream, pTypesGraph);
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_BinarySerializer);
