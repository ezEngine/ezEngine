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

ezResult ezAnimGraph::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(6);

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    inout_stream << node->GetDynamicRTTI()->GetTypeName();

    EZ_SUCCEED_OR_RETURN(node->SerializeNode(inout_stream));
  }

  inout_stream << m_hSkeleton;

  {
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_TriggerInputPinStates));

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_NumberInputPinStates));

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Number].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Number])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_BoolInputPinStates));

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Bool])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_BoneWeightInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_LocalPoseInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_ModelPoseInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose])
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return EZ_SUCCESS;
}

ezResult ezAnimGraph::Deserialize(ezStreamReader& inout_stream, ezArrayPtr<ezUniquePtr<ezAnimGraphNode>> allNodes)
{
  const auto uiVersion = inout_stream.ReadVersion(6);

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

  inout_stream >> m_hSkeleton;

  if (uiVersion >= 2)
  {
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_TriggerInputPinStates));

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
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_NumberInputPinStates));

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
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_BoolInputPinStates));

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

void ezAnimGraph::SetInstanceDataAllocator(ezInstanceDataAllocator& ref_allocator)
{
  m_pInstanceDataAllocator = &ref_allocator;
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


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
