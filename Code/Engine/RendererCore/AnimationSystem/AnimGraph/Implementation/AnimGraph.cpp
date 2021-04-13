#include <RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>

ezMutex ezAnimGraph::s_SharedDataMutex;
ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> ezAnimGraph::s_SharedBoneWeights;

ezAnimGraph::ezAnimGraph()
{
  m_pBlackboard = &m_Blackboard;
}

ezAnimGraph::~ezAnimGraph() = default;

void ezAnimGraph::Update(ezTime tDiff, ezGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  m_pCurrentModelTransforms = nullptr;

  m_PoseGenerator.Reset(pSkeleton.GetPointer());

  // reset all pin states
  {
    m_PinDataBoneWeights.Clear();
    m_PinDataLocalTransforms.Clear();
    m_PinDataModelTransforms.Clear();

    for (auto& pin : m_TriggerInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_NumberInputPinStates)
    {
      pin = 0;
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
    pNode->Step(*this, tDiff, pSkeleton.GetPointer(), pTarget);
  }

  if (auto newPose = GetPoseGenerator().GeneratePose(); !newPose.IsEmpty())
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    pTarget->SendMessageRecursive(msg);
  }
}

ezVec3 ezAnimGraph::GetRootMotion() const
{
  if (m_pCurrentModelTransforms && m_pCurrentModelTransforms->m_bUseRootMotion)
  {
    return m_pCurrentModelTransforms->m_vRootMotion;
  }

  return ezVec3::ZeroVector();
}

void ezAnimGraph::SetExternalBlackboard(ezBlackboard* pBlackboard)
{
  if (pBlackboard)
  {
    m_pBlackboard = pBlackboard;
  }
  else
  {
    m_pBlackboard = &m_Blackboard;
  }
}

ezResult ezAnimGraph::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(5);

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    stream << node->GetDynamicRTTI()->GetTypeName();

    EZ_SUCCEED_OR_RETURN(node->SerializeNode(stream));
  }

  stream << m_hSkeleton;

  EZ_SUCCEED_OR_RETURN(m_Blackboard.Serialize(stream));

  {
    EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_TriggerInputPinStates));

    stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger])
    {
      EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
    }
  }
  {
    EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_NumberInputPinStates));

    stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::Number].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Number])
    {
      EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
    }
  }
  {
    stream << m_BoneWeightInputPinStates.GetCount();

    stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights])
    {
      EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
    }
  }
  {
    stream << m_LocalPoseInputPinStates.GetCount();

    stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose])
    {
      EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
    }
  }
  {
    stream << m_ModelPoseInputPinStates.GetCount();

    stream << m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose])
    {
      EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return EZ_SUCCESS;
}

ezResult ezAnimGraph::Deserialize(ezStreamReader& stream)
{
  const auto uiVersion = stream.ReadVersion(5);

  ezUInt32 uiNumNodes = 0;
  stream >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  ezStringBuilder sTypeName;

  for (auto& node : m_Nodes)
  {
    stream >> sTypeName;
    node = std::move(ezRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<ezAnimGraphNode>());

    EZ_SUCCEED_OR_RETURN(node->DeserializeNode(stream));
  }

  stream >> m_hSkeleton;

  EZ_SUCCEED_OR_RETURN(m_Blackboard.Deserialize(stream));

  if (uiVersion >= 2)
  {
    EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_TriggerInputPinStates));

    ezUInt32 sar = 0;
    stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger])
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 3)
  {
    EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_NumberInputPinStates));

    ezUInt32 sar = 0;
    stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::Number].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::Number])
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 4)
  {
    ezUInt32 sar = 0;

    stream >> sar;
    m_BoneWeightInputPinStates.SetCount(sar);

    stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights])
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    ezUInt32 sar = 0;

    stream >> sar;
    m_LocalPoseInputPinStates.SetCount(sar);

    stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose])
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    ezUInt32 sar = 0;

    stream >> sar;
    m_ModelPoseInputPinStates.SetCount(sar);

    stream >> sar;
    m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose])
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return EZ_SUCCESS;
}

ezAnimGraphPinDataBoneWeights* ezAnimGraph::AddPinDataBoneWeights()
{
  ezAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = m_PinDataBoneWeights.GetCount() - 1;
  return pData;
}

ezAnimGraphPinDataLocalTransforms* ezAnimGraph::AddPinDataLocalTransforms()
{
  ezAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = m_PinDataLocalTransforms.GetCount() - 1;
  return pData;
}

ezAnimGraphPinDataModelTransforms* ezAnimGraph::AddPinDataModelTransforms()
{
  ezAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = m_PinDataModelTransforms.GetCount() - 1;
  return pData;
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
