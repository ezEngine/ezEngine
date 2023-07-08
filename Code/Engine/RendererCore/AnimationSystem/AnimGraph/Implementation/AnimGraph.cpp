#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton.h>

ezMutex ezAnimGraphInstance::s_SharedDataMutex;
ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> ezAnimGraphInstance::s_SharedBoneWeights;

ezAnimGraphInstance::ezAnimGraphInstance() = default;

ezAnimGraphInstance::~ezAnimGraphInstance()
{
  if (m_pAnimGraph)
  {
    m_pAnimGraph->GetInstanceDataAlloator().DestructAndDeallocate(m_InstanceData);
  }
}

void ezAnimGraphInstance::Configure(const ezAnimGraph& animGraph, const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard /*= nullptr*/)
{
  m_pAnimGraph = &animGraph;
  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;

  m_InstanceData = m_pAnimGraph->GetInstanceDataAlloator().AllocateAndConstruct();

  // EXTEND THIS if a new type is introduced
  m_pTriggerInputPinStates = (ezInt8*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Trigger]);
  m_pNumberInputPinStates = (double*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Number]);
  m_pBoolInputPinStates = (bool*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Bool]);
  m_pBoneWeightInputPinStates = (ezUInt16*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::BoneWeights]);
  m_pModelPoseInputPinStates = (ezUInt16*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::ModelPose]);

  m_LocalPoseInputPinStates.SetCount(animGraph.m_uiInputPinCounts[ezAnimGraphPin::Type::LocalPose]);
}

void ezAnimGraphInstance::Update(ezTime diff, ezGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  m_pCurrentModelTransforms = nullptr;

  m_CurrentLocalTransformOutputs.Clear();

  m_vRootMotion = ezVec3::ZeroVector();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_pPoseGenerator->Reset(pSkeleton.GetPointer());

  // reset all pin states
  {
    m_PinDataBoneWeights.Clear();
    m_PinDataLocalTransforms.Clear();
    m_PinDataModelTransforms.Clear();

    // EXTEND THIS if a new type is introduced

    ezMemoryUtils::ZeroFill(m_pTriggerInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Trigger]);
    ezMemoryUtils::ZeroFill(m_pNumberInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Number]);
    ezMemoryUtils::ZeroFill(m_pBoolInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Bool]);
    ezMemoryUtils::ZeroFill(m_pBoneWeightInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::BoneWeights]);
    ezMemoryUtils::PatternFill(m_pModelPoseInputPinStates, 0xFF, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::ModelPose]);

    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
  }

  for (const auto& pNode : m_pAnimGraph->GetNodes())
  {
    pNode->Step(*this, diff, pSkeleton.GetPointer(), pTarget);
  }

  GenerateLocalResultProcessors(pSkeleton.GetPointer());

  if (auto newPose = GetPoseGenerator().GeneratePose(pTarget); !newPose.IsEmpty())
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    // TODO: root transform has to be applied first, only then can the world-space IK be done, and then the pose can be finalized
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;

    // recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments
    pTarget->SendMessageRecursive(msg);
  }
}

void ezAnimGraphInstance::GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

ezAnimGraphPinDataBoneWeights* ezAnimGraphInstance::AddPinDataBoneWeights()
{
  ezAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataLocalTransforms* ezAnimGraphInstance::AddPinDataLocalTransforms()
{
  ezAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataModelTransforms* ezAnimGraphInstance::AddPinDataModelTransforms()
{
  ezAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void ezAnimGraphInstance::SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void ezAnimGraphInstance::SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

void ezAnimGraphInstance::AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms)
{
  m_CurrentLocalTransformOutputs.PushBack(pLocalTransforms->m_uiOwnIndex);
}

ezSharedPtr<ezAnimGraphSharedBoneWeights> ezAnimGraphInstance::CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill)
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

void ezAnimGraphInstance::GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton)
{
  if (m_CurrentLocalTransformOutputs.IsEmpty())
    return;

  ezAnimGraphPinDataLocalTransforms* pOut = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[0]];

  // combine multiple outputs
  if (m_CurrentLocalTransformOutputs.GetCount() > 1 || pOut->m_pWeights != nullptr)
  {
    const ezUInt32 m_uiMaxPoses = 6; // TODO

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

      const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[i]];

      if (pTransforms != nullptr)
      {
        pw[i].m_fPinWeight = pTransforms->m_fOverallWeight;

        if (pTransforms->m_pWeights)
        {
          pw[i].m_fPinWeight *= pTransforms->m_pWeights->m_fOverallWeight;
        }
      }
    }

    if (pw.GetCount() > m_uiMaxPoses)
    {
      pw.Sort([](const PinWeight& lhs, const PinWeight& rhs)
        { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
      pw.SetCount(m_uiMaxPoses);
    }

    ezArrayPtr<const ozz::math::SimdFloat4> invWeights;

    for (const auto& in : pw)
    {
      const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

      if (in.m_fPinWeight > 0 && pTransforms->m_pWeights)
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

        const ezArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

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
        const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

        if (pTransforms->m_pWeights)
        {
          const ezArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

          cmd.m_InputBoneWeights.PushBack(weights);
        }
        else
        {
          cmd.m_InputBoneWeights.PushBack(invWeights);
        }

        if (pTransforms->m_bUseRootMotion)
        {
          fSummedRootMotionWeight += in.m_fPinWeight;
          pOut->m_vRootMotion += pTransforms->m_vRootMotion * in.m_fPinWeight;

          // TODO: combining quaternions is mathematically tricky
          // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

          pOut->m_bUseRootMotion = true;
        }

        cmd.m_Inputs.PushBack(pTransforms->m_CommandID);
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
    GetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);

    auto& cmd = GetPoseGenerator().AllocCommandModelPoseToOutput();
    cmd.m_Inputs.PushBack(pModelTransform->m_CommandID);

    if (pModelTransform->m_bUseRootMotion)
    {
      rootMotion += pModelTransform->m_vRootMotion;
      rootRotationX += pModelTransform->m_RootRotationX;
      rootRotationY += pModelTransform->m_RootRotationY;
      rootRotationZ += pModelTransform->m_RootRotationZ;
    }

    SetOutputModelTransform(pModelTransform);

    SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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

void ezAnimGraph::AddConnection(const ezAnimGraphNode* pSrcNode, ezStringView sSrcPinName, const ezAnimGraphNode* pDstNode, ezStringView sDstPinName)
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
    desc.m_uiTypeAlignment = EZ_ALIGNMENT_OF(ezInt8);
    desc.m_uiTypeSize = sizeof(ezInt8) * m_uiInputPinCounts[ezAnimGraphPin::Type::Trigger];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Trigger] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = EZ_ALIGNMENT_OF(double);
    desc.m_uiTypeSize = sizeof(double) * m_uiInputPinCounts[ezAnimGraphPin::Type::Number];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Number] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = EZ_ALIGNMENT_OF(bool);
    desc.m_uiTypeSize = sizeof(bool) * m_uiInputPinCounts[ezAnimGraphPin::Type::Bool];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Bool] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = EZ_ALIGNMENT_OF(ezUInt16);
    desc.m_uiTypeSize = sizeof(ezUInt16) * m_uiInputPinCounts[ezAnimGraphPin::Type::BoneWeights];
    m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::BoneWeights] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    ezInstanceDataDesc desc;
    desc.m_uiTypeAlignment = EZ_ALIGNMENT_OF(ezUInt16);
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
