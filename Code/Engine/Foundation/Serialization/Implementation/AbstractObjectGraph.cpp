#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectChangeType, 1)
  EZ_ENUM_CONSTANTS(ezObjectChangeType::NodeAdded, ezObjectChangeType::NodeRemoved)
  EZ_ENUM_CONSTANTS(ezObjectChangeType::PropertySet, ezObjectChangeType::PropertyInserted, ezObjectChangeType::PropertyRemoved)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAbstractObjectNode, ezNoBase, 1, ezRTTIDefaultAllocator<ezAbstractObjectNode>)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezDiffOperation, ezNoBase, 1, ezRTTIDefaultAllocator<ezDiffOperation>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Operation", ezObjectChangeType, m_Operation),
    EZ_MEMBER_PROPERTY("Node", m_Node),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
    EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

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


ezAbstractObjectNode* ezAbstractObjectGraph::Clone(ezAbstractObjectGraph& ref_cloneTarget, const ezAbstractObjectNode* pRootNode, FilterFunction filter) const
{
  ref_cloneTarget.Clear();

  if (pRootNode == nullptr)
  {
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (filter.IsValid())
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value(), filter);
      }
      else
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value());
      }
    }
    return nullptr;
  }
  else
  {
    EZ_ASSERT_DEV(pRootNode->GetOwner() == this, "The given root node must be part of this document");
    ezSet<ezUuid> reachableNodes;
    FindTransitiveHull(pRootNode->GetGuid(), reachableNodes);

    for (const ezUuid& guid : reachableNodes)
    {
      if (auto* pNode = GetNode(guid))
      {
        if (filter.IsValid())
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode, filter);
        }
        else
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode);
        }
      }
    }

    return ref_cloneTarget.GetNode(pRootNode->GetGuid());
  }
}

ezStringView ezAbstractObjectGraph::RegisterString(ezStringView sString)
{
  auto it = m_Strings.Insert(sString);
  EZ_ASSERT_DEV(it.IsValid(), "");
  return it.Key();
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid)
{
  return m_Nodes.GetValueOrDefault(guid, nullptr);
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid) const
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNode(guid);
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(ezStringView sName) const
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNodeByName(sName);
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(ezStringView sName)
{
  return m_NodesByName.GetValueOrDefault(sName, nullptr);
}

ezAbstractObjectNode* ezAbstractObjectGraph::AddNode(const ezUuid& guid, ezStringView sType, ezUInt32 uiTypeVersion, ezStringView sNodeName)
{
  EZ_ASSERT_DEV(!m_Nodes.Contains(guid), "object {0} must not yet exist", guid);
  if (!sNodeName.IsEmpty())
  {
    sNodeName = RegisterString(sNodeName);
  }
  else
  {
    sNodeName = {};
  }

  ezAbstractObjectNode* pNode = EZ_DEFAULT_NEW(ezAbstractObjectNode);
  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_sType = RegisterString(sType);
  pNode->m_uiTypeVersion = uiTypeVersion;
  pNode->m_sNodeName = sNodeName;

  m_Nodes[guid] = pNode;

  if (!sNodeName.IsEmpty())
  {
    m_NodesByName[sNodeName] = pNode;
  }

  return pNode;
}

void ezAbstractObjectGraph::RemoveNode(const ezUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    ezAbstractObjectNode* pNode = it.Value();
    if (!pNode->m_sNodeName.IsEmpty())
      m_NodesByName.Remove(pNode->m_sNodeName);

    m_Nodes.Remove(guid);
    EZ_DEFAULT_DELETE(pNode);
  }
}

void ezAbstractObjectNode::AddProperty(ezStringView sName, const ezVariant& value)
{
  auto& prop = m_Properties.ExpandAndGetRef();
  prop.m_sPropertyName = m_pOwner->RegisterString(sName);
  prop.m_Value = value;
}

void ezAbstractObjectNode::ChangeProperty(ezStringView sName, const ezVariant& value)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  EZ_REPORT_FAILURE("Property '{0}' is unknown", sName);
}

void ezAbstractObjectNode::RenameProperty(ezStringView sOldName, ezStringView sNewName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sOldName)
    {
      m_Properties[i].m_sPropertyName = m_pOwner->RegisterString(sNewName);
      return;
    }
  }
}

void ezAbstractObjectNode::ClearProperties()
{
  m_Properties.Clear();
}

ezResult ezAbstractObjectNode::InlineProperty(ezStringView sName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    Property& prop = m_Properties[i];
    if (prop.m_sPropertyName == sName)
    {
      if (!prop.m_Value.IsA<ezUuid>())
        return EZ_FAILURE;

      ezUuid guid = prop.m_Value.Get<ezUuid>();
      ezAbstractObjectNode* pNode = m_pOwner->GetNode(guid);
      if (!pNode)
        return EZ_FAILURE;

      class InlineContext : public ezRttiConverterContext
      {
      public:
        void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject) override
        {
          EZ_IGNORE_UNUSED(pRtti);
          EZ_IGNORE_UNUSED(pObject);
          m_SubTree.PushBack(guid);
        }
        ezHybridArray<ezUuid, 1> m_SubTree;
      };

      InlineContext context;
      ezRttiConverterReader reader(m_pOwner, &context);
      void* pObject = reader.CreateObjectFromNode(pNode);
      if (!pObject)
        return EZ_FAILURE;

      prop.m_Value.MoveTypedObject(pObject, ezRTTI::FindTypeByName(pNode->GetType()));

      // Delete old objects.
      for (ezUuid& uuid : context.m_SubTree)
      {
        m_pOwner->RemoveNode(uuid);
      }
      return EZ_SUCCESS;
    }
  }
  return EZ_FAILURE;
}

void ezAbstractObjectNode::RemoveProperty(ezStringView sName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties.RemoveAtAndSwap(i);
      return;
    }
  }
}

void ezAbstractObjectNode::SetType(ezStringView sType)
{
  m_sType = m_pOwner->RegisterString(sType);
}

const ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(ezStringView sName) const
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(ezStringView sName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

void ezAbstractObjectGraph::ReMapNodeGuids(const ezUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  ezHybridArray<ezAbstractObjectNode*, 16> nodes;
  nodes.Reserve(m_Nodes.GetCount());
  ezHashTable<ezUuid, ezUuid> guidMap;
  guidMap.Reserve(m_Nodes.GetCount());

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


void ezAbstractObjectGraph::ReMapNodeGuidsToMatchGraph(ezAbstractObjectNode* pRoot, const ezAbstractObjectGraph& rhsGraph, const ezAbstractObjectNode* pRhsRoot)
{
  ezHashTable<ezUuid, ezUuid> guidMap;
  EZ_ASSERT_DEV(pRoot->GetType() == pRhsRoot->GetType(), "Roots must have the same type to be able re-map guids!");

  ReMapNodeGuidsToMatchGraphRecursive(guidMap, pRoot, rhsGraph, pRhsRoot);

  // go through all nodes to remap remaining occurrences of remapped guids
  for (auto it : m_Nodes)
  {
    // check every property
    for (auto& prop : it.Value()->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[it.Value()->m_Guid] = it.Value();
  }
}

void ezAbstractObjectGraph::ReMapNodeGuidsToMatchGraphRecursive(ezHashTable<ezUuid, ezUuid>& guidMap, ezAbstractObjectNode* lhs, const ezAbstractObjectGraph& rhsGraph, const ezAbstractObjectNode* rhs)
{
  if (lhs->GetType() != rhs->GetType())
  {
    // Types differ, remapping ends as this is a removal and add of a new object.
    return;
  }

  if (lhs->GetGuid() != rhs->GetGuid())
  {
    guidMap[lhs->GetGuid()] = rhs->GetGuid();
    m_Nodes.Remove(lhs->GetGuid());
    lhs->m_Guid = rhs->GetGuid();
    m_Nodes.Insert(rhs->GetGuid(), lhs);
  }

  for (ezAbstractObjectNode::Property& prop : lhs->m_Properties)
  {
    if (prop.m_Value.IsA<ezUuid>() && prop.m_Value.Get<ezUuid>().IsValid())
    {
      // if the guid is an owned object in the graph, remap to rhs.
      auto it = m_Nodes.Find(prop.m_Value.Get<ezUuid>());
      if (it.IsValid())
      {
        if (const ezAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
        {
          if (rhsProp->m_Value.IsA<ezUuid>() && rhsProp->m_Value.Get<ezUuid>().IsValid())
          {
            if (const ezAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsProp->m_Value.Get<ezUuid>()))
            {
              ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
            }
          }
        }
      }
    }
    // Arrays may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<ezVariantArray>())
    {
      const ezVariantArray& values = prop.m_Value.Get<ezVariantArray>();
      for (ezUInt32 i = 0; i < values.GetCount(); i++)
      {
        auto& subValue = values[i];
        if (subValue.IsA<ezUuid>() && subValue.Get<ezUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to array element.
          auto it = m_Nodes.Find(subValue.Get<ezUuid>());
          if (it.IsValid())
          {
            if (const ezAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<ezVariantArray>())
              {
                const ezVariantArray& rhsValues = rhsProp->m_Value.Get<ezVariantArray>();
                if (i < rhsValues.GetCount())
                {
                  const auto& rhsElemValue = rhsValues[i];
                  if (rhsElemValue.IsA<ezUuid>() && rhsElemValue.Get<ezUuid>().IsValid())
                  {
                    if (const ezAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<ezUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // Maps may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<ezVariantDictionary>())
    {
      const ezVariantDictionary& values = prop.m_Value.Get<ezVariantDictionary>();
      for (auto lhsIt = values.GetIterator(); lhsIt.IsValid(); ++lhsIt)
      {
        auto& subValue = lhsIt.Value();
        if (subValue.IsA<ezUuid>() && subValue.Get<ezUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to map element.
          auto it = m_Nodes.Find(subValue.Get<ezUuid>());
          if (it.IsValid())
          {
            if (const ezAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<ezVariantDictionary>())
              {
                const ezVariantDictionary& rhsValues = rhsProp->m_Value.Get<ezVariantDictionary>();
                if (rhsValues.Contains(lhsIt.Key()))
                {
                  const auto& rhsElemValue = *rhsValues.GetValue(lhsIt.Key());
                  if (rhsElemValue.IsA<ezUuid>() && rhsElemValue.Get<ezUuid>().IsValid())
                  {
                    if (const ezAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<ezUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


void ezAbstractObjectGraph::FindTransitiveHull(const ezUuid& rootGuid, ezSet<ezUuid>& ref_reachableNodes) const
{
  ref_reachableNodes.Clear();
  ezSet<ezUuid> inProgress;
  inProgress.Insert(rootGuid);

  while (!inProgress.IsEmpty())
  {
    ezUuid current = *inProgress.GetIterator();
    auto it = m_Nodes.Find(current);
    if (it.IsValid())
    {
      const ezAbstractObjectNode* pNode = it.Value();
      for (auto& prop : pNode->m_Properties)
      {
        if (prop.m_Value.IsA<ezUuid>())
        {
          const ezUuid& guid = prop.m_Value.Get<ezUuid>();
          if (!ref_reachableNodes.Contains(guid))
          {
            inProgress.Insert(guid);
          }
        }
        // Arrays may be of uuids
        else if (prop.m_Value.IsA<ezVariantArray>())
        {
          const ezVariantArray& values = prop.m_Value.Get<ezVariantArray>();
          for (auto& subValue : values)
          {
            if (subValue.IsA<ezUuid>())
            {
              const ezUuid& guid = subValue.Get<ezUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
        else if (prop.m_Value.IsA<ezVariantDictionary>())
        {
          const ezVariantDictionary& values = prop.m_Value.Get<ezVariantDictionary>();
          for (auto& subValue : values)
          {
            if (subValue.Value().IsA<ezUuid>())
            {
              const ezUuid& guid = subValue.Value().Get<ezUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
      }
    }
    // Even if 'current' is not in the graph add it anyway to early out if it is found again.
    ref_reachableNodes.Insert(current);
    inProgress.Remove(current);
  }
}

void ezAbstractObjectGraph::PruneGraph(const ezUuid& rootGuid)
{
  ezSet<ezUuid> reachableNodes;
  FindTransitiveHull(rootGuid, reachableNodes);

  // Determine nodes to be removed by subtracting valid ones from all nodes.
  ezSet<ezUuid> removeSet;
  for (auto it = GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    removeSet.Insert(it.Key());
  }
  removeSet.Difference(reachableNodes);

  // Remove nodes.
  for (const ezUuid& guid : removeSet)
  {
    RemoveNode(guid);
  }
}

void ezAbstractObjectGraph::ModifyNodeViaNativeCounterpart(ezAbstractObjectNode* pRootNode, ezDelegate<void(void*, const ezRTTI*)> callback)
{
  EZ_ASSERT_DEV(pRootNode->GetOwner() == this, "Node must be from this graph.");

  // Clone sub graph
  ezAbstractObjectGraph origGraph;
  ezAbstractObjectNode* pOrigRootNode = nullptr;
  {
    pOrigRootNode = Clone(origGraph, pRootNode);
  }

  // Create native object
  ezRttiConverterContext context;
  ezRttiConverterReader convRead(&origGraph, &context);
  void* pNativeRoot = convRead.CreateObjectFromNode(pOrigRootNode);
  const ezRTTI* pType = ezRTTI::FindTypeByName(pOrigRootNode->GetType());
  EZ_SCOPE_EXIT(pType->GetAllocator()->Deallocate(pNativeRoot););

  // Make changes to native object
  if (callback.IsValid())
  {
    callback(pNativeRoot, pType);
  }

  // Create native object graph
  ezAbstractObjectGraph graph;
  {
    // The ezApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
    // of the object manager.
    ezApplyNativePropertyChangesContext nativeChangesContext(context, origGraph);
    ezRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
    nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), pType, pNativeRoot);
    rttiConverter.AddObjectToGraph(pType, pNativeRoot, "Object");
  }

  // Create diff from native to cloned sub-graph and then apply the diff to the original graph.
  ezDeque<ezAbstractGraphDiffOperation> diffResult;
  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  ApplyDiff(diffResult);
}

ezAbstractObjectNode* ezAbstractObjectGraph::CopyNodeIntoGraph(const ezAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
  {
    pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

ezAbstractObjectNode* ezAbstractObjectGraph::CopyNodeIntoGraph(const ezAbstractObjectNode* pNode, FilterFunction& ref_filter)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  if (ref_filter.IsValid())
  {
    for (const auto& props : pNode->GetProperties())
    {
      if (!ref_filter(pNode, &props))
        continue;

      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
    }
  }
  else
  {
    for (const auto& props : pNode->GetProperties())
      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

void ezAbstractObjectGraph::CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_diffResult) const
{
  out_diffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base
        ezAbstractGraphDiffOperation op;
        op.m_Node = itNodeBase.Key();
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeRemoved;
        op.m_sProperty = itNodeBase.Value()->m_sType;
        op.m_Value = itNodeBase.Value()->m_sNodeName;

        out_diffResult.PushBack(op);
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
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeAdded;
        op.m_sProperty = itNodeThis.Value()->m_sType;
        op.m_Value = itNodeThis.Value()->m_sNodeName;

        out_diffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
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

      for (const ezAbstractObjectNode::Property& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const ezAbstractObjectNode::Property& baseProp : pBaseNode->GetProperties())
        {
          if (baseProp.m_sPropertyName == prop.m_sPropertyName)
          {
            if (baseProp.m_Value == prop.m_Value)
            {
              bDifferent = false;
              break;
            }

            bDifferent = true;
            break;
          }
        }

        if (bDifferent)
        {
          ezAbstractGraphDiffOperation op;
          op.m_Node = itNodeThis.Key();
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }
}


void ezAbstractObjectGraph::ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& ref_diff)
{
  for (const auto& op : ref_diff)
  {
    switch (op.m_Operation)
    {
      case ezAbstractGraphDiffOperation::Op::NodeAdded:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_uiTypeVersion, op.m_Value.Get<ezString>());
      }
      break;

      case ezAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        RemoveNode(op.m_Node);
      }
      break;

      case ezAbstractGraphDiffOperation::Op::PropertyChanged:
      {
        auto* pNode = GetNode(op.m_Node);
        if (pNode)
        {
          auto* pProp = pNode->FindProperty(op.m_sProperty);

          if (!pProp)
            pNode->AddProperty(op.m_sProperty, op.m_Value);
          else
            pProp->m_Value = op.m_Value;
        }
      }
      break;
    }
  }
}


void ezAbstractObjectGraph::MergeDiffs(const ezDeque<ezAbstractGraphDiffOperation>& lhs, const ezDeque<ezAbstractGraphDiffOperation>& rhs, ezDeque<ezAbstractGraphDiffOperation>& ref_out) const
{
  struct Prop
  {
    Prop() = default;
    Prop(ezUuid node, ezStringView sProperty)
      : m_Node(node)
      , m_sProperty(sProperty)
    {
    }
    ezUuid m_Node;
    ezStringView m_sProperty;

    bool operator<(const Prop& rhs) const
    {
      if (m_Node == rhs.m_Node)
        return m_sProperty < rhs.m_sProperty;

      return m_Node < rhs.m_Node;
    }

    bool operator==(const Prop& rhs) const { return m_Node == rhs.m_Node && m_sProperty == rhs.m_sProperty; }
  };

  ezMap<Prop, ezHybridArray<const ezAbstractGraphDiffOperation*, 2>> propChanges;
  ezSet<ezUuid> removed;
  ezMap<ezUuid, ezUInt32> added;
  for (const ezAbstractGraphDiffOperation& op : lhs)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      removed.Insert(op.m_Node);
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeAdded)
    {
      added[op.m_Node] = ref_out.GetCount();
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }
  for (const ezAbstractGraphDiffOperation& op : rhs)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      if (!removed.Contains(op.m_Node))
        ref_out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeAdded)
    {
      if (added.Contains(op.m_Node))
      {
        ezAbstractGraphDiffOperation& leftOp = ref_out[added[op.m_Node]];
        leftOp.m_sProperty = op.m_sProperty; // Take type from rhs.
      }
      else
      {
        ref_out.PushBack(op);
      }
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }

  for (auto it = propChanges.GetIterator(); it.IsValid(); ++it)
  {
    const Prop& key = it.Key();
    const ezHybridArray<const ezAbstractGraphDiffOperation*, 2>& value = it.Value();

    if (value.GetCount() == 1)
    {
      ref_out.PushBack(*value[0]);
    }
    else
    {
      const ezAbstractGraphDiffOperation& leftProp = *value[0];
      const ezAbstractGraphDiffOperation& rightProp = *value[1];

      if (leftProp.m_Value.GetType() == ezVariantType::VariantArray && rightProp.m_Value.GetType() == ezVariantType::VariantArray)
      {
        const ezVariantArray& leftArray = leftProp.m_Value.Get<ezVariantArray>();
        const ezVariantArray& rightArray = rightProp.m_Value.Get<ezVariantArray>();

        const ezAbstractObjectNode* pNode = GetNode(key.m_Node);
        if (pNode)
        {
          ezStringBuilder sTemp(key.m_sProperty);
          const ezAbstractObjectNode::Property* pProperty = pNode->FindProperty(sTemp);
          if (pProperty && pProperty->m_Value.GetType() == ezVariantType::VariantArray)
          {
            // Do 3-way array merge
            const ezVariantArray& baseArray = pProperty->m_Value.Get<ezVariantArray>();
            ezVariantArray res;
            MergeArrays(baseArray, leftArray, rightArray, res);
            ref_out.PushBack(rightProp);
            ref_out.PeekBack().m_Value = res;
          }
          else
          {
            ref_out.PushBack(rightProp);
          }
        }
        else
        {
          ref_out.PushBack(rightProp);
        }
      }
      else
      {
        ref_out.PushBack(rightProp);
      }
    }
  }
}

void ezAbstractObjectGraph::RemapVariant(ezVariant& value, const ezHashTable<ezUuid, ezUuid>& guidMap)
{
  ezStringBuilder tmp;

  // if the property is a guid, we check if we need to remap it
  if (value.IsA<ezUuid>())
  {
    const ezUuid& guid = value.Get<ezUuid>();

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = *found;
    }
  }
  else if (value.IsA<ezString>() && ezConversionUtils::IsStringUuid(value.Get<ezString>()))
  {
    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(value.Get<ezString>());

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = ezConversionUtils::ToString(*found, tmp).GetData();
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
      else if (subValue.IsA<ezString>() && ezConversionUtils::IsStringUuid(subValue.Get<ezString>()))
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
  // Maps may be of uuids
  else if (value.IsA<ezVariantDictionary>())
  {
    const ezVariantDictionary& values = value.Get<ezVariantDictionary>();
    bool bNeedToRemap = false;
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      const ezVariant& subValue = it.Value();

      if (subValue.IsA<ezUuid>() && guidMap.Contains(subValue.Get<ezUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<ezString>() && ezConversionUtils::IsStringUuid(subValue.Get<ezString>()))
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      ezVariantDictionary newValues = values;
      for (auto it = newValues.GetIterator(); it.IsValid(); ++it)
      {
        RemapVariant(it.Value(), guidMap);
      }
      value = newValues;
    }
  }
}

void ezAbstractObjectGraph::MergeArrays(const ezDynamicArray<ezVariant>& baseArray, const ezDynamicArray<ezVariant>& leftArray, const ezDynamicArray<ezVariant>& rightArray, ezDynamicArray<ezVariant>& out) const
{
  // Find element type.
  ezVariantType::Enum type = ezVariantType::Invalid;
  if (!baseArray.IsEmpty())
    type = baseArray[0].GetType();
  if (type != ezVariantType::Invalid && !leftArray.IsEmpty())
    type = leftArray[0].GetType();
  if (type != ezVariantType::Invalid && !rightArray.IsEmpty())
    type = rightArray[0].GetType();

  if (type == ezVariantType::Invalid)
    return;

  // For now, assume non-uuid types are arrays, uuids are sets.
  if (type != ezVariantType::Uuid)
  {
    // Any size changes?
    ezUInt32 uiSize = baseArray.GetCount();
    if (leftArray.GetCount() != baseArray.GetCount())
      uiSize = leftArray.GetCount();
    if (rightArray.GetCount() != baseArray.GetCount())
      uiSize = rightArray.GetCount();

    out.SetCount(uiSize);
    for (ezUInt32 i = 0; i < uiSize; i++)
    {
      if (i < baseArray.GetCount())
        out[i] = baseArray[i];
    }

    ezUInt32 uiCountLeft = ezMath::Min(uiSize, leftArray.GetCount());
    for (ezUInt32 i = 0; i < uiCountLeft; i++)
    {
      if (leftArray[i] != baseArray[i])
        out[i] = leftArray[i];
    }

    ezUInt32 uiCountRight = ezMath::Min(uiSize, rightArray.GetCount());
    for (ezUInt32 i = 0; i < uiCountRight; i++)
    {
      if (rightArray[i] != baseArray[i])
        out[i] = rightArray[i];
    }
    return;
  }

  // Move distance is NP-complete so try greedy algorithm
  struct Element
  {
    Element(const ezVariant* pValue = nullptr, ezInt32 iBaseIndex = -1, ezInt32 iLeftIndex = -1, ezInt32 iRightIndex = -1)
      : m_pValue(pValue)
      , m_iBaseIndex(iBaseIndex)
      , m_iLeftIndex(iLeftIndex)
      , m_iRightIndex(iRightIndex)
      , m_fIndex(ezMath::MaxValue<float>())
    {
    }
    bool IsDeleted() const { return m_iBaseIndex != -1 && (m_iLeftIndex == -1 || m_iRightIndex == -1); }
    bool operator<(const Element& rhs) const { return m_fIndex < rhs.m_fIndex; }

    const ezVariant* m_pValue;
    ezInt32 m_iBaseIndex;
    ezInt32 m_iLeftIndex;
    ezInt32 m_iRightIndex;
    float m_fIndex;
  };
  ezDynamicArray<Element> baseOrder;
  baseOrder.Reserve(leftArray.GetCount() + rightArray.GetCount());

  // First, add up all unique elements and their position in each array.
  for (ezInt32 i = 0; i < (ezInt32)baseArray.GetCount(); i++)
  {
    baseOrder.PushBack(Element(&baseArray[i], i));
    baseOrder.PeekBack().m_fIndex = (float)i;
  }

  ezDynamicArray<ezInt32> leftOrder;
  leftOrder.SetCountUninitialized(leftArray.GetCount());
  for (ezInt32 i = 0; i < (ezInt32)leftArray.GetCount(); i++)
  {
    const ezVariant& val = leftArray[i];
    bool bFound = false;
    for (ezInt32 j = 0; j < (ezInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iLeftIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iLeftIndex = i;
        leftOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      leftOrder[i] = (ezInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&leftArray[i], -1, i));
    }
  }

  ezDynamicArray<ezInt32> rightOrder;
  rightOrder.SetCountUninitialized(rightArray.GetCount());
  for (ezInt32 i = 0; i < (ezInt32)rightArray.GetCount(); i++)
  {
    const ezVariant& val = rightArray[i];
    bool bFound = false;
    for (ezInt32 j = 0; j < (ezInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iRightIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iRightIndex = i;
        rightOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      rightOrder[i] = (ezInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&rightArray[i], -1, -1, i));
    }
  }

  // Re-order greedy
  float fLastElement = -0.5f;
  for (ezInt32 i = 0; i < (ezInt32)leftOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[leftOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = ezMath::MaxValue<float>();
    for (ezInt32 j = i + 1; j < (ezInt32)leftOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[leftOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }

  fLastElement = -0.5f;
  for (ezInt32 i = 0; i < (ezInt32)rightOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[rightOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = ezMath::MaxValue<float>();
    for (ezInt32 j = i + 1; j < (ezInt32)rightOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[rightOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }


  // Sort
  baseOrder.Sort();
  out.Reserve(baseOrder.GetCount());
  for (const Element& elem : baseOrder)
  {
    if (!elem.IsDeleted())
    {
      out.PushBack(*elem.m_pValue);
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);
