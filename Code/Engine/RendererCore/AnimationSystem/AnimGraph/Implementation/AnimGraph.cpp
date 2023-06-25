#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton.h>

ezMutex ezAnimGraph::s_SharedDataMutex;
ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> ezAnimGraph::s_SharedBoneWeights;

ezAnimGraph::ezAnimGraph() = default;

ezAnimGraph::~ezAnimGraph()
{
  if (m_pInstanceDataAllocator)
  {
    m_pInstanceDataAllocator->DestructAndDeallocate(m_InstanceData);
  }
}

void ezAnimGraph::Configure(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard /*= nullptr*/)
{
  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;
}

void ezAnimGraph::Update(ezTime diff, ezGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  if (!m_bInitialized)
  {
    m_bInitialized = true;

    EZ_LOG_BLOCK("Initializing animation controller graph");

    for (const auto& pNode : m_Nodes)
    {
      pNode->Initialize(*this, pSkeleton.GetPointer());
    }
  }

  m_pCurrentModelTransforms = nullptr;

  m_CurrentLocalTransformOutputs.Clear();

  m_pPoseGenerator->Reset(pSkeleton.GetPointer());

  // reset all pin states
  {
    m_PinDataBoneWeights.Clear();
    m_PinDataLocalTransforms.Clear();
    m_PinDataModelTransforms.Clear();

    // EXTEND THIS if a new type is introduced

    for (auto& pin : m_TriggerInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_NumberInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_BoolInputPinStates)
    {
      pin = false;
    }
    for (auto& pin : m_BoneWeightInputPinStates)
    {
      pin = 0xFFFF;
    }
    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
    for (auto& pin : m_ModelPoseInputPinStates)
    {
      pin = 0xFFFF;
    }
  }

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(*this, diff, pSkeleton.GetPointer(), pTarget);
  }

  GenerateLocalResultProcessors(pSkeleton.GetPointer());

  if (auto newPose = GetPoseGenerator().GeneratePose(pTarget); !newPose.IsEmpty())
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    // recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments
    pTarget->SendMessageRecursive(msg);
  }
}

void ezAnimGraph::GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

ezResult ezAnimGraph::Deserialize(ezStreamReader& inout_stream, ezArrayPtr<ezUniquePtr<ezAnimGraphNode>> allNodes)
{
  const auto uiVersion = inout_stream.ReadVersion(7);

  ezUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  ezStringBuilder sTypeName;

  for (ezUInt32 i = 0; i < uiNumNodes; ++i)
  {
    auto& node = m_Nodes[i];

    inout_stream >> sTypeName;
    node = std::move(ezRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<ezAnimGraphNode>());

    EZ_SUCCEED_OR_RETURN(node->DeserializeNode(inout_stream));
    node->m_uiInstanceDataOffset = allNodes[i]->m_uiInstanceDataOffset;
  }

  if (uiVersion < 7)
  {
    inout_stream >> m_hSkeleton;
  }

  if (uiVersion >= 2)
  {
    if (uiVersion >= 7)
    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;
      m_TriggerInputPinStates.SetCount(uiCount);
    }
    else
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_TriggerInputPinStates));
    }

    ezUInt32 sar = 0;
    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 3)
  {
    if (uiVersion >= 7)
    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;
      m_NumberInputPinStates.SetCount(uiCount);
    }
    else
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_NumberInputPinStates));
    }

    ezUInt32 sar = 0;
    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::Number].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Number])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 6)
  {
    if (uiVersion >= 7)
    {
      ezUInt32 uiCount = 0;
      inout_stream >> uiCount;
      m_BoolInputPinStates.SetCount(uiCount);
    }
    else
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_BoolInputPinStates));
    }

    ezUInt32 sar = 0;
    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 4)
  {
    ezUInt32 sar = 0;

    inout_stream >> sar;
    m_BoneWeightInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    ezUInt32 sar = 0;

    inout_stream >> sar;
    m_LocalPoseInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    ezUInt32 sar = 0;

    inout_stream >> sar;
    m_ModelPoseInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return EZ_SUCCESS;
}

ezAnimGraphPinDataBoneWeights* ezAnimGraph::AddPinDataBoneWeights()
{
  ezAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataLocalTransforms* ezAnimGraph::AddPinDataLocalTransforms()
{
  ezAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataModelTransforms* ezAnimGraph::AddPinDataModelTransforms()
{
  ezAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void ezAnimGraph::SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void ezAnimGraph::SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

void ezAnimGraph::AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms)
{
  m_CurrentLocalTransformOutputs.PushBack(pLocalTransforms);
}

void ezAnimGraph::SetInstanceDataAllocator(const ezInstanceDataAllocator& allocator)
{
  m_pInstanceDataAllocator = &allocator;
  m_InstanceData = m_pInstanceDataAllocator->AllocateAndConstruct();
}

ezSharedPtr<ezAnimGraphSharedBoneWeights> ezAnimGraph::CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill)
{
  EZ_LOCK(s_SharedDataMutex);

  ezSharedPtr<ezAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = EZ_DEFAULT_NEW(ezAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    ezMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}

void ezAnimGraph::GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton)
{
  if (m_CurrentLocalTransformOutputs.IsEmpty())
    return;

  ezAnimGraphPinDataLocalTransforms* pOut = m_CurrentLocalTransformOutputs[0];

  // combine multiple outputs
  if (m_CurrentLocalTransformOutputs.GetCount() > 1)
  {
    const ezUInt32 m_uiMaxPoses = 6; // TODO

    // TODO move somewhere else
    ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_BlendMask;

    pOut = AddPinDataLocalTransforms();
    pOut->m_vRootMotion.SetZero();

    float fSummedRootMotionWeight = 0.0f;

    // TODO: skip blending, if only a single animation is played
    // unless the weight is below 1.0 and the bind pose should be faded in

    auto& cmd = GetPoseGenerator().AllocCommandCombinePoses();

    struct PinWeight
    {
      ezUInt32 m_uiPinIdx;
      float m_fPinWeight = 0.0f;
    };

    ezHybridArray<PinWeight, 16> pw;
    pw.SetCount(m_CurrentLocalTransformOutputs.GetCount());

    for (ezUInt32 i = 0; i < m_CurrentLocalTransformOutputs.GetCount(); ++i)
    {
      pw[i].m_uiPinIdx = i;

      if (m_CurrentLocalTransformOutputs[i] != nullptr)
      {
        pw[i].m_fPinWeight = m_CurrentLocalTransformOutputs[i]->m_fOverallWeight;

        if (m_CurrentLocalTransformOutputs[i]->m_pWeights)
        {
          pw[i].m_fPinWeight *= m_CurrentLocalTransformOutputs[i]->m_pWeights->m_fOverallWeight;
        }
      }
    }

    if (pw.GetCount() > m_uiMaxPoses)
    {
      pw.Sort([](const PinWeight& lhs, const PinWeight& rhs) { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
      pw.SetCount(m_uiMaxPoses);
    }

    ezArrayPtr<const ozz::math::SimdFloat4> invWeights;

    for (const auto& in : pw)
    {
      if (in.m_fPinWeight > 0 && m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_pWeights)
      {
        // only initialize and use the inverse mask, when it is actually needed
        if (invWeights.IsEmpty())
        {
          m_BlendMask.SetCountUninitialized(pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

          for (auto& sj : m_BlendMask)
          {
            sj = ozz::math::simd_float4::one();
          }

          invWeights = m_BlendMask;
        }

        const ozz::math::SimdFloat4 factor = ozz::math::simd_float4::Load1(in.m_fPinWeight);

        const ezArrayPtr<const ozz::math::SimdFloat4> weights = m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_pWeights->m_pSharedBoneWeights->m_Weights;

        for (ezUInt32 i = 0; i < m_BlendMask.GetCount(); ++i)
        {
          const auto& weight = weights[i];
          auto& mask = m_BlendMask[i];

          const auto oneMinusWeight = ozz::math::NMAdd(factor, weight, ozz::math::simd_float4::one());

          mask = ozz::math::Min(mask, oneMinusWeight);
        }
      }
    }

    for (const auto& in : pw)
    {
      if (in.m_fPinWeight > 0)
      {
        if (m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_pWeights)
        {
          const ezArrayPtr<const ozz::math::SimdFloat4> weights = m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_pWeights->m_pSharedBoneWeights->m_Weights;

          cmd.m_InputBoneWeights.PushBack(weights);
        }
        else
        {
          cmd.m_InputBoneWeights.PushBack(invWeights);
        }

        if (m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_bUseRootMotion)
        {
          fSummedRootMotionWeight += in.m_fPinWeight;
          pOut->m_vRootMotion += m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_vRootMotion * in.m_fPinWeight;

          // TODO: combining quaternions is mathematically tricky
          // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

          pOut->m_bUseRootMotion = true;
        }

        cmd.m_Inputs.PushBack(m_CurrentLocalTransformOutputs[in.m_uiPinIdx]->m_CommandID);
        cmd.m_InputWeights.PushBack(in.m_fPinWeight);
      }
    }

    if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
    {
      pOut->m_vRootMotion /= fSummedRootMotionWeight;
    }

    pOut->m_CommandID = cmd.GetCommandID();
  }

  ezAnimGraphPinDataModelTransforms* pModelTransform = AddPinDataModelTransforms();

  // local space to model space
  {
    if (pOut->m_bUseRootMotion)
    {
      pModelTransform->m_bUseRootMotion = true;
      pModelTransform->m_vRootMotion = pOut->m_vRootMotion;
    }

    auto& cmd = GetPoseGenerator().AllocCommandLocalToModelPose();
    cmd.m_Inputs.PushBack(pOut->m_CommandID);

    pModelTransform->m_CommandID = cmd.GetCommandID();
  }

  // model space to output
  {
    ezVec3 rootMotion = ezVec3::ZeroVector();
    ezAngle rootRotationX;
    ezAngle rootRotationY;
    ezAngle rootRotationZ;

    auto& cmd = GetPoseGenerator().AllocCommandModelPoseToOutput();
    cmd.m_Inputs.PushBack(pModelTransform->m_CommandID);

    if (pModelTransform->m_bUseRootMotion)
    {
      rootMotion = pModelTransform->m_vRootMotion;
      rootRotationX = pModelTransform->m_RootRotationX;
      rootRotationY = pModelTransform->m_RootRotationY;
      rootRotationZ = pModelTransform->m_RootRotationZ;
    }

    SetOutputModelTransform(pModelTransform);

    SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAnimGraphBuilder::ezAnimGraphBuilder()
{
  ezMemoryUtils::ZeroFillArray(m_uiInputPinCounts);
}

ezAnimGraphBuilder::~ezAnimGraphBuilder() = default;

ezAnimGraphNode* ezAnimGraphBuilder::AddNode(ezUniquePtr<ezAnimGraphNode>&& pNode)
{
  m_bPreparedForUse = false;

  m_Nodes.PushBack(std::move(pNode));
  return m_Nodes.PeekBack().Borrow();
}

void ezAnimGraphBuilder::AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, const ezAnimGraphNode* pDstNode, ezStringView sDstPinName)
{
  // TODO: assert pSrcNode and pDstNode exist

  m_bPreparedForUse = false;

  ezAbstractMemberProperty* pPinPropSrc = (ezAbstractMemberProperty*)pSrcNode->GetDynamicRTTI()->FindPropertyByName(sSrcPinName);
  ezAbstractMemberProperty* pPinPropDst = (ezAbstractMemberProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);

  auto& to = m_From[pSrcNode].m_To.ExpandAndGetRef();
  to.m_sSrcPinName = sSrcPinName;
  to.m_pDstNode = pDstNode;
  to.m_sDstPinName = sDstPinName;
  to.m_pSrcPin = (ezAnimGraphPin*)pPinPropSrc->GetPropertyPointer(pSrcNode);
  to.m_pDstPin = (ezAnimGraphPin*)pPinPropDst->GetPropertyPointer(pDstNode);
}

void ezAnimGraphBuilder::PreparePinMapping()
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

void ezAnimGraphBuilder::AssignInputPinIndices()
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

void ezAnimGraphBuilder::AssignOutputPinIndices()
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

ezUInt16 ezAnimGraphBuilder::ComputeNodePriority(const ezAnimGraphNode* pNode, ezMap<const ezAnimGraphNode*, ezUInt16>& inout_Prios, ezUInt16& inout_uiOutputPrio) const
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

void ezAnimGraphBuilder::SortNodesByPriority()
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

  m_Nodes.Sort([&](const auto& lhs, const auto& rhs) -> bool { return prios[lhs.Borrow()] < prios[rhs.Borrow()]; });
}

void ezAnimGraphBuilder::PrepareForUse()
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
}

ezResult ezAnimGraphBuilder::SerializeForUse(ezStreamWriter& inout_stream)
{
  PrepareForUse();

  inout_stream.WriteVersion(7);

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    inout_stream << node->GetDynamicRTTI()->GetTypeName();

    EZ_SUCCEED_OR_RETURN(node->SerializeNode(inout_stream));
  }

  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::Trigger];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::Number];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Number].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Number])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::Bool];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::BoneWeights];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::LocalPose];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_uiInputPinCounts[ezAnimGraphPin::ModelPose];

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return EZ_SUCCESS;
}

ezResult ezAnimGraphBuilder::Serialize(ezStreamWriter& inout_stream) const
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

ezResult ezAnimGraphBuilder::Deserialize(ezStreamReader& inout_stream)
{
  const ezTypeVersion version = inout_stream.ReadVersion(10);

  if (version < 10)
    return EZ_FAILURE;

  ezUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;

  ezDynamicArray<const ezAnimGraphNode*> idxToNode;
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
      const ezAnimGraphNode* ptrNodeTo = idxToNode[nodeIdx];

      inout_stream >> sPinDst;

      AddConnection(ptrNodeFrom, sPinSrc, ptrNodeTo, sPinDst);
    }
  }

  m_bPreparedForUse = false;
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
