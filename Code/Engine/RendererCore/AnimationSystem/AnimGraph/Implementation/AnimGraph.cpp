#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

ezAnimGraph::ezAnimGraph()
{
  Clear();
}

ezAnimGraph::~ezAnimGraph() = default;

void ezAnimGraph::Clear()
{
  ezMemoryUtils::ZeroFillArray(m_uiInputPinCounts);
  ezMemoryUtils::ZeroFillArray(m_uiPinInstanceDataOffset);
  m_From.Clear();
  m_Nodes.Clear();
  m_bPreparedForUse = true;
  m_InstanceDataAllocator.ClearDescs();

  for (auto& r : m_OutputPinToInputPinMapping)
  {
    r.Clear();
  }
}

ezAnimGraphNode* ezAnimGraph::AddNode(ezUniquePtr<ezAnimGraphNode>&& pNode)
{
  m_bPreparedForUse = false;

  m_Nodes.PushBack(std::move(pNode));
  return m_Nodes.PeekBack().Borrow();
}

void ezAnimGraph::AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, ezAnimGraphNode* pDstNode, ezStringView sDstPinName)
{
  // TODO: assert pSrcNode and pDstNode exist

  m_bPreparedForUse = false;
  ezStringView sIdx;

  ezAbstractMemberProperty* pPinPropSrc = (ezAbstractMemberProperty*)pSrcNode->GetDynamicRTTI()->FindPropertyByName(sSrcPinName);

  auto& to = m_From[pSrcNode].m_To.ExpandAndGetRef();
  to.m_sSrcPinName = sSrcPinName;
  to.m_pDstNode = pDstNode;
  to.m_sDstPinName = sDstPinName;
  to.m_pSrcPin = (ezAnimGraphPin*)pPinPropSrc->GetPropertyPointer(pSrcNode);

  if (const char* szIdx = sDstPinName.FindSubString("["))
  {
    sIdx = ezStringView(szIdx + 1, sDstPinName.GetEndPointer() - 1);
    sDstPinName = ezStringView(sDstPinName.GetStartPointer(), szIdx);

    ezAbstractArrayProperty* pPinPropDst = (ezAbstractArrayProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);
    const ezDynamicPinAttribute* pDynPinAttr = pPinPropDst->GetAttributeByType<ezDynamicPinAttribute>();

    const ezTypedMemberProperty<ezUInt8>* pPinSizeProp = (const ezTypedMemberProperty<ezUInt8>*)pDstNode->GetDynamicRTTI()->FindPropertyByName(pDynPinAttr->GetProperty());
    ezUInt8 uiArraySize = pPinSizeProp->GetValue(pDstNode);
    pPinPropDst->SetCount(pDstNode, uiArraySize);

    ezUInt32 uiIdx;
    ezConversionUtils::StringToUInt(sIdx, uiIdx).AssertSuccess();

    to.m_pDstPin = (ezAnimGraphPin*)pPinPropDst->GetValuePointer(pDstNode, uiIdx);
  }
  else
  {
    ezAbstractMemberProperty* pPinPropDst = (ezAbstractMemberProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);

    to.m_pDstPin = (ezAnimGraphPin*)pPinPropDst->GetPropertyPointer(pDstNode);
  }
}

void ezAnimGraph::PreparePinMapping()
{
  ezUInt16 uiOutputPinCounts[ezAnimGraphPin::Type::ENUM_COUNT];
  ezMemoryUtils::ZeroFillArray(uiOutputPinCounts);

  for (const auto& consFrom : m_From)
  {
    for (const ConnectionTo& to : consFrom.Value().m_To)
    {
      uiOutputPinCounts[to.m_pSrcPin->GetPinType()]++;
    }
  }

  for (ezUInt32 i = 0; i < ezAnimGraphPin::ENUM_COUNT; ++i)
  {
    m_OutputPinToInputPinMapping[i].Clear();
    m_OutputPinToInputPinMapping[i].SetCount(uiOutputPinCounts[i]);
  }
}

void ezAnimGraph::AssignInputPinIndices()
{
  ezMemoryUtils::ZeroFillArray(m_uiInputPinCounts);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      // there may be multiple connections to this pin
      // only assign the index the first time we see a connection to this pin
      // otherwise only count up the number of connections

      if (to.m_pDstPin->m_iPinIndex == -1)
      {
        to.m_pDstPin->m_iPinIndex = m_uiInputPinCounts[to.m_pDstPin->GetPinType()]++;
      }

      ++to.m_pDstPin->m_uiNumConnections;
    }
  }
}

void ezAnimGraph::AssignOutputPinIndices()
{
  ezInt16 iPinTypeCount[ezAnimGraphPin::Type::ENUM_COUNT];
  ezMemoryUtils::ZeroFillArray(iPinTypeCount);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      const ezUInt8 pinType = to.m_pSrcPin->GetPinType();

      // there may be multiple connections from this pin
      // only assign the index the first time we see a connection from this pin

      if (to.m_pSrcPin->m_iPinIndex == -1)
      {
        to.m_pSrcPin->m_iPinIndex = iPinTypeCount[pinType]++;
      }

      // store the indices of all the destination pins
      m_OutputPinToInputPinMapping[pinType][to.m_pSrcPin->m_iPinIndex].PushBack(to.m_pDstPin->m_iPinIndex);
    }
  }
}

ezUInt16 ezAnimGraph::ComputeNodePriority(const ezAnimGraphNode* pNode, ezMap<const ezAnimGraphNode*, ezUInt16>& inout_Prios, ezUInt16& inout_uiOutputPrio) const
{
  auto itPrio = inout_Prios.Find(pNode);
  if (itPrio.IsValid())
  {
    // priority already computed -> return it
    return itPrio.Value();
  }

  const auto itConsFrom = m_From.Find(pNode);

  ezUInt16 uiOwnPrio = 0xFFFF;

  if (itConsFrom.IsValid())
  {
    // look at all outgoing priorities and take the smallest dst priority - 1
    for (const ConnectionTo& to : itConsFrom.Value().m_To)
    {
      uiOwnPrio = ezMath::Min<ezUInt16>(uiOwnPrio, ComputeNodePriority(to.m_pDstNode, inout_Prios, inout_uiOutputPrio) - 1);
    }
  }
  else
  {
    // has no outgoing connections at all -> max priority value
    uiOwnPrio = inout_uiOutputPrio;
    inout_uiOutputPrio -= 64;
  }

  EZ_ASSERT_DEBUG(uiOwnPrio != 0xFFFF, "");

  inout_Prios[pNode] = uiOwnPrio;
  return uiOwnPrio;
}

void ezAnimGraph::SortNodesByPriority()
{
  // this is important so that we can step all nodes in linear order,
  // and have them generate their output such that it is ready before
  // dependent nodes are stepped

  ezUInt16 uiOutputPrio = 0xFFFE;
  ezMap<const ezAnimGraphNode*, ezUInt16> prios;
  for (const auto& pNode : m_Nodes)
  {
    ComputeNodePriority(pNode.Borrow(), prios, uiOutputPrio);
  }

  m_Nodes.Sort([&](const auto& lhs, const auto& rhs) -> bool
    { return prios[lhs.Borrow()] < prios[rhs.Borrow()]; });
}

void ezAnimGraph::PrepareForUse()
{
  if (m_bPreparedForUse)
    return;

  m_bPreparedForUse = true;

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      to.m_pSrcPin->m_iPinIndex = -1;
      to.m_pSrcPin->m_uiNumConnections = 0;
      to.m_pDstPin->m_iPinIndex = -1;
      to.m_pDstPin->m_uiNumConnections = 0;
    }
  }

  SortNodesByPriority();
  PreparePinMapping();
  AssignInputPinIndices();
  AssignOutputPinIndices();

  m_InstanceDataAllocator.ClearDescs();
  for (const auto& pNode : m_Nodes)
  {
    ezInstanceDataDesc desc;
    if (pNode->GetInstanceDataDesc(desc))
    {
      pNode->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
    }
  }

  // EXTEND THIS if a new type is introduced
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(ezInt8);
    desc.m_uiTypeSize = sizeof(ezInt8) * m_uiInputPinCounts[ezAnimGraphPin::Type::Trigger];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Trigger] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(double);
    desc.m_uiTypeSize = sizeof(double) * m_uiInputPinCounts[ezAnimGraphPin::Type::Number];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Number] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(bool);
    desc.m_uiTypeSize = sizeof(bool) * m_uiInputPinCounts[ezAnimGraphPin::Type::Bool];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Bool] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(ezUInt16);
    desc.m_uiTypeSize = sizeof(ezUInt16) * m_uiInputPinCounts[ezAnimGraphPin::Type::BoneWeights];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::BoneWeights] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = alignof(ezUInt16);
    desc.m_uiTypeSize = sizeof(ezUInt16) * m_uiInputPinCounts[ezAnimGraphPin::Type::ModelPose];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::ModelPose] = m_InstanceDataAllocator.AddDesc(desc);
  }
}

ezResult ezAnimGraph::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(10);

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  ezMap<const ezAnimGraphNode*, ezUInt32> nodeToIdx;

  for (ezUInt32 n = 0; n < m_Nodes.GetCount(); ++n)
  {
    const ezAnimGraphNode* pNode = m_Nodes[n].Borrow();

    nodeToIdx[pNode] = n;

    inout_stream << pNode->GetDynamicRTTI()->GetTypeName();
    EZ_SUCCEED_OR_RETURN(pNode->SerializeNode(inout_stream));
  }

  inout_stream << m_From.GetCount();
  for (auto itFrom : m_From)
  {
    inout_stream << nodeToIdx[itFrom.Key()];

    const auto& toAll = itFrom.Value().m_To;
    inout_stream << toAll.GetCount();

    for (const auto& to : toAll)
    {
      inout_stream << to.m_sSrcPinName;
      inout_stream << nodeToIdx[to.m_pDstNode];
      inout_stream << to.m_sDstPinName;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezAnimGraph::Deserialize(ezStreamReader& inout_stream)
{
  Clear();

  const ezTypeVersion version = inout_stream.ReadVersion(10);

  if (version < 10)
    return EZ_FAILURE;

  ezUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;

  ezDynamicArray<ezAnimGraphNode*> idxToNode;
  idxToNode.SetCount(uiNumNodes);

  ezStringBuilder sTypeName;

  for (ezUInt32 n = 0; n < uiNumNodes; ++n)
  {
    inout_stream >> sTypeName;
    ezUniquePtr<ezAnimGraphNode> pNode = ezRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<ezAnimGraphNode>();
    EZ_SUCCEED_OR_RETURN(pNode->DeserializeNode(inout_stream));

    idxToNode[n] = AddNode(std::move(pNode));
  }

  ezUInt32 uiNumConnectionsFrom = 0;
  inout_stream >> uiNumConnectionsFrom;

  ezStringBuilder sPinSrc, sPinDst;

  for (ezUInt32 cf = 0; cf < uiNumConnectionsFrom; ++cf)
  {
    ezUInt32 nodeIdx;
    inout_stream >> nodeIdx;
    const ezAnimGraphNode* ptrNodeFrom = idxToNode[nodeIdx];

    ezUInt32 uiNumConnectionsTo = 0;
    inout_stream >> uiNumConnectionsTo;

    for (ezUInt32 ct = 0; ct < uiNumConnectionsTo; ++ct)
    {
      inout_stream >> sPinSrc;

      inout_stream >> nodeIdx;
      ezAnimGraphNode* ptrNodeTo = idxToNode[nodeIdx];

      inout_stream >> sPinDst;

      AddConnection(ptrNodeFrom, sPinSrc, ptrNodeTo, sPinDst);
    }
  }

  m_bPreparedForUse = false;
  return EZ_SUCCESS;
}
