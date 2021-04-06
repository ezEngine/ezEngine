#include <RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>

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

  // reset all pin states
  {
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
      pin = nullptr;
    }
    for (ezHybridArray<ezAnimGraphLocalTransforms*, 1>& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
    for (auto& pin : m_ModelPoseInputPinStates)
    {
      pin = nullptr;
    }
  }

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(*this, tDiff, pSkeleton.GetPointer(), pTarget);
  }

  if (m_pCurrentModelTransforms == nullptr) // nothing generated
    return;

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_pCurrentModelTransforms->m_ModelTransforms;

  pTarget->SendMessageRecursive(msg);
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

ezAnimGraphBoneWeights* ezAnimGraph::AllocateBoneWeights(const ezSkeletonResource& skeleton)
{
  ezAnimGraphBoneWeights* pWeights = nullptr;

  if (!m_BoneWeightsFreeList.IsEmpty())
  {
    pWeights = m_BoneWeightsFreeList.PeekBack();
    m_BoneWeightsFreeList.PopBack();
  }
  else
  {
    pWeights = &m_BoneWeights.ExpandAndGetRef();
  }

  pWeights->m_ozzBoneWeights.resize(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

  return pWeights;
}

void ezAnimGraph::FreeBoneWeights(ezAnimGraphBoneWeights*& pWeights)
{
  if (pWeights == nullptr)
    return;

  m_BoneWeightsFreeList.PushBack(pWeights);
  pWeights = nullptr;
}

ezAnimGraphLocalTransforms* ezAnimGraph::AllocateLocalTransforms(const ezSkeletonResource& skeleton)
{
  ezAnimGraphLocalTransforms* pTransforms = nullptr;

  if (!m_LocalTransformsFreeList.IsEmpty())
  {
    pTransforms = m_LocalTransformsFreeList.PeekBack();
    m_LocalTransformsFreeList.PopBack();
  }
  else
  {
    pTransforms = &m_LocalTransforms.ExpandAndGetRef();
  }

  pTransforms->m_ozzLocalTransforms.resize(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

  return pTransforms;
}

void ezAnimGraph::FreeLocalTransforms(ezAnimGraphLocalTransforms*& pTransforms)
{
  if (pTransforms == nullptr)
    return;

  m_LocalTransformsFreeList.PushBack(pTransforms);
  pTransforms = nullptr;
}

ezAnimGraphModelTransforms* ezAnimGraph::AllocateModelTransforms(const ezSkeletonResource& skeleton)
{
  ezAnimGraphModelTransforms* pTransforms = nullptr;

  if (!m_ModelTransformsFreeList.IsEmpty())
  {
    pTransforms = m_ModelTransformsFreeList.PeekBack();
    m_ModelTransformsFreeList.PopBack();
  }
  else
  {
    pTransforms = &m_ModelTransforms.ExpandAndGetRef();
  }

  pTransforms->m_ModelTransforms.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return pTransforms;
}

void ezAnimGraph::FreeModelTransforms(ezAnimGraphModelTransforms*& pTransforms)
{
  if (pTransforms == nullptr)
    return;

  m_ModelTransformsFreeList.PushBack(pTransforms);
  pTransforms = nullptr;
}

ezAnimGraphSamplingCache* ezAnimGraph::AllocateSamplingCache(const ozz::animation::Animation& animclip)
{
  ezAnimGraphSamplingCache* pCache = nullptr;

  if (!m_SamplingCachesFreeList.IsEmpty())
  {
    pCache = m_SamplingCachesFreeList.PeekBack();
    m_SamplingCachesFreeList.PopBack();
  }
  else
  {
    pCache = &m_SamplingCaches.ExpandAndGetRef();
  }

  pCache->m_pUsedForAnim = &animclip;
  pCache->m_ozzSamplingCache.Resize(animclip.num_tracks());

  return pCache;
}

void ezAnimGraph::UpdateSamplingCache(ezAnimGraphSamplingCache*& pCache, const ozz::animation::Animation& animclip)
{
  if (pCache == nullptr)
  {
    pCache = AllocateSamplingCache(animclip);
    return;
  }

  if (pCache->m_pUsedForAnim != &animclip)
  {
    pCache->m_pUsedForAnim = &animclip;
    pCache->m_ozzSamplingCache.Resize(animclip.num_tracks());
    return;
  }
}

void ezAnimGraph::FreeSamplingCache(ezAnimGraphSamplingCache*& pCache)
{
  if (pCache == nullptr)
    return;

  pCache->m_pUsedForAnim = nullptr;
  pCache->m_ozzSamplingCache.Invalidate();
  m_SamplingCachesFreeList.PushBack(pCache);
  pCache = nullptr;
}
