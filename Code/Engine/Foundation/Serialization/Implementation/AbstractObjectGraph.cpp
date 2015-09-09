#include <Foundation/PCH.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

ezAbstractObjectGraph::~ezAbstractObjectGraph()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    EZ_DEFAULT_DELETE(it.Value());
  }
}

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
    return it.Value();

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
  EZ_ASSERT_DEV(!m_Nodes.Contains(guid), "object must not yet exist");
  if (szNodeName != nullptr)
  {
    szNodeName = RegisterString(szNodeName);
  }

  ezAbstractObjectNode* pNode = EZ_DEFAULT_NEW(ezAbstractObjectNode);
  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_szType = RegisterString(szType);
  pNode->m_szNodeName = szNodeName;

  m_Nodes[guid] = pNode;

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
    ezAbstractObjectNode* pNode = it.Value();
    if (pNode->m_szNodeName != nullptr)
      m_NodesByName.Remove(pNode->m_szNodeName);

    m_Nodes.Remove(guid);
    EZ_DEFAULT_DELETE(pNode);
  }
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


//
//void ezAbstractObjectGraph::ChangeNodeGuid(const ezUuid& oldGuid, const ezUuid& newGuid)
//{
//  ezAbstractObjectNode& oldNode = m_Nodes[oldGuid];
//  oldNode.m_Guid = newGuid;
//
//  ezAbstractObjectNode& newNode = m_Nodes[newGuid];
//  newNode = oldNode;
//
//  m_Nodes.Remove(oldGuid);
//
//  m_NodesByName[newNode.m_szNodeName] = &newNode;
//}

void ezAbstractObjectGraph::ReMapNodeGuids(const ezUuid& seedGuid)
{
  ezHybridArray<ezAbstractObjectNode*, 16> nodes;
  ezMap<ezUuid, ezUuid> guidMap;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    ezUuid newGuid = it.Key();
    newGuid.CombineWithSeed(seedGuid);

    guidMap[it.Key()] = newGuid;

    nodes.PushBack(it.Value());
  }

  m_Nodes.Clear();

  // go through all nodes to remap guids
  for (auto* pNode : nodes)
  {
    pNode->m_Guid = guidMap[pNode->m_Guid];

    // check every property
    for (auto& prop : pNode->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
      
    }
    m_Nodes[pNode->m_Guid] = pNode;
  }

}

void ezAbstractObjectGraph::RemapVariant(ezVariant& value, const ezMap<ezUuid, ezUuid>& guidMap)
{
  // if the property is a guid, we check if we need to remap it
  if (value.IsA<ezUuid>())
  {
    const ezUuid& guid = value.Get<ezUuid>();

    // if we find the guid in our map, replace it by the new guid
    auto it = guidMap.Find(guid);

    if (it.IsValid())
    {
      value = it.Value();
    }
  }
  // Arrays may be of uuids
  else if (value.IsA<ezVariantArray>())
  {
    const ezVariantArray& values = value.Get<ezVariantArray>();
    bool bNeedToRemap = false;
    for (auto& subValue : values)
    {
      if (subValue.IsA<ezUuid>() && guidMap.Contains(subValue.Get<ezUuid>()))
      {
        bNeedToRemap = true;
      }
      else if (subValue.IsA<ezVariantArray>())
      {
        bNeedToRemap = true;
      }
    }

    if (bNeedToRemap)
    {
      ezVariantArray newValues = values;
      for (auto& subValue : newValues)
      {
        RemapVariant(subValue, guidMap);
      }
      value = newValues;
    }
  }
}