#include <Foundation/PCH.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

const char* ezAbstractObjectGraph::RegisterString(const char* szString)
{
  auto it = m_Strings.Insert(szString);
  EZ_ASSERT_DEV(it.IsValid(), "");
  return it.Key().GetData();
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid)
{
  auto it = m_Nodes.Find(guid);
  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid) const 
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNode(guid);
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(const char* szName) const
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNodeByName(szName);
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(const char* szName)
{
  auto it = m_Strings.Find(szName);
  if (!it.IsValid())
    return nullptr;

  auto itNode = m_NodesByName.Find(it.Key().GetData());
  if (!itNode.IsValid())
    return nullptr;

  return itNode.Value();
}

ezAbstractObjectNode* ezAbstractObjectGraph::AddNode(const ezUuid& guid, const char* szType, const char* szNodeName)
{
  if (szNodeName != nullptr)
  {
    szNodeName = RegisterString(szNodeName);
  }

  auto* pNode = &m_Nodes[guid];
  EZ_ASSERT_DEV(!pNode->m_Guid.IsValid(), "object must not yet exist");

  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_szType = RegisterString(szType);
  pNode->m_szNodeName = szNodeName;

  if (szNodeName != nullptr)
  {
    m_NodesByName[szNodeName] = pNode;
  }

  return pNode;
}

void ezAbstractObjectGraph::RemoveNode(const ezUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    if (it.Value().m_szNodeName != nullptr)
      m_NodesByName.Remove(it.Value().m_szNodeName);

    m_Nodes.Remove(guid);
  }
}


void ezAbstractObjectGraph::ChangeNodeGuid(const ezUuid& oldGuid, const ezUuid& newGuid)
{
  ezAbstractObjectNode& oldNode = m_Nodes[oldGuid];
  oldNode.m_Guid = newGuid;

  ezAbstractObjectNode& newNode = m_Nodes[newGuid];
  newNode = oldNode;

  m_Nodes.Remove(oldGuid);

  m_NodesByName[newNode.m_szNodeName] = &newNode;
}

void ezAbstractObjectNode::AddProperty(const char* szName, const ezVariant& value)
{
  auto& prop = m_Properties.ExpandAndGetRef();
  prop.m_szPropertyName = m_pOwner->RegisterString(szName);
  prop.m_Value = value;
}

void ezAbstractObjectNode::ChangeProperty(const char* szName, const ezVariant& value)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  EZ_REPORT_FAILURE("Property '%s' is unknown", szName);
}

void ezAbstractObjectNode::RemoveProperty(const char* szName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      m_Properties.RemoveAtSwap(i);
      return;
    }
  }
}

const ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(const char* szName) const
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}