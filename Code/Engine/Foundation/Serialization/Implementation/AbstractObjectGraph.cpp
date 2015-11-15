#include <Foundation/PCH.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

ezAbstractObjectGraph::~ezAbstractObjectGraph()
{
  Clear();
}

void ezAbstractObjectGraph::Clear()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    EZ_DEFAULT_DELETE(it.Value());
  }
  m_Nodes.Clear();
  m_NodesByName.Clear();
  m_Strings.Clear();
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

ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(const char* szName)
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

void ezAbstractObjectGraph::ReMapNodeGuids(const ezUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  ezHybridArray<ezAbstractObjectNode*, 16> nodes;
  ezMap<ezUuid, ezUuid> guidMap;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    ezUuid newGuid = it.Key();

    if (bRemapInverse)
      newGuid.RevertCombinationWithSeed(seedGuid);
    else
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

void ezAbstractObjectGraph::CopyNodeIntoGraph(ezAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
    pNewNode->AddProperty(props.m_szPropertyName, props.m_Value);
}


void ezAbstractObjectGraph::CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_DiffResult)
{
  out_DiffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base

        ezAbstractGraphDiffOperation op;
        op.m_Node = itNodeBase.Key();
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeDelete;

        out_DiffResult.PushBack(op);
      }
    }
  }

  // check whether any nodes have been added
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      if (base.GetNode(itNodeThis.Key()) == nullptr)
      {
        // does not exist in base graph -> has been added

        ezAbstractGraphDiffOperation op;
        op.m_Node = itNodeThis.Key();
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeAdd;
        op.m_sProperty = itNodeThis.Value()->m_szType;
        op.m_Value = itNodeThis.Value()->m_szNodeName;

        out_DiffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertySet;
          op.m_sProperty = prop.m_szPropertyName;
          op.m_Value = prop.m_Value;

          out_DiffResult.PushBack(op);
        }
      }
    }
  }

  // check whether any properties have been modified
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      const auto pBaseNode = base.GetNode(itNodeThis.Key());

      if (pBaseNode == nullptr)
        continue;

      for (const auto& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const auto& baseProp : pBaseNode->GetProperties())
        {
          if (ezStringUtils::IsEqual(baseProp.m_szPropertyName, prop.m_szPropertyName))
          {
            if (baseProp.m_Value == prop.m_Value) /// \todo Handle arrays, etc.
              bDifferent = false;

            break;
          }
        }

        if (bDifferent)
        {
          ezAbstractGraphDiffOperation op;
          op.m_Node = itNodeThis.Key();
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertySet;
          op.m_sProperty = prop.m_szPropertyName;
          op.m_Value = prop.m_Value;

          out_DiffResult.PushBack(op);
        }

        /// \todo Handle deleted properties ?
      }
    }
  }
}


void ezAbstractObjectGraph::ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& Diff)
{
  for (const auto& op : Diff)
  {
    switch (op.m_Operation)
    {
    case ezAbstractGraphDiffOperation::Op::NodeAdd:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_Value.Get<ezString>());
      }
      break;

    case ezAbstractGraphDiffOperation::Op::NodeDelete:
      {
        RemoveNode(op.m_Node);
      }
      break;

    case ezAbstractGraphDiffOperation::Op::PropertySet:
      {
        auto* pNode = GetNode(op.m_Node);

        auto* pProp = pNode->FindProperty(op.m_sProperty);

        if (!pProp)
          pNode->AddProperty(op.m_sProperty, op.m_Value);
        else
          pProp->m_Value = op.m_Value;
      }
      break;
    }
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
        break;
      }
      else if (subValue.IsA<ezVariantArray>())
      {
        bNeedToRemap = true;
        break;
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



EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);

