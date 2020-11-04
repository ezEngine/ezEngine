#include <RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>

ezAnimGraph::ezAnimGraph() = default;
ezAnimGraph::~ezAnimGraph() = default;

void ezAnimGraph::Update(ezTime tDiff)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  m_ozzBlendLayers.Clear();

  m_vRootMotion.SetZero();

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(this, tDiff, pSkeleton.GetPointer());
  }

  {
    m_ozzLocalTransforms.resize(pOzzSkeleton->num_soa_joints());

    ozz::animation::BlendingJob job;
    job.threshold = 0.1f;
    job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(m_ozzBlendLayers), end(m_ozzBlendLayers));
    job.bind_pose = pOzzSkeleton->joint_bind_poses();
    job.output = make_span(m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  m_bFinalized = false;
}

void ezAnimGraph::Finalize(const ezSkeletonResource* pSkeleton)
{
  if (m_bFinalized)
    return;

  m_bFinalized = true;

  const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  {
    m_ModelSpaceTransforms.SetCountUninitialized(pOzzSkeleton->num_joints());

    ozz::animation::LocalToModelJob job;
    job.input = make_span(m_ozzLocalTransforms);
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(begin(m_ModelSpaceTransforms)), reinterpret_cast<ozz::math::Float4x4*>(end(m_ModelSpaceTransforms)));
    job.skeleton = pOzzSkeleton;
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }
}

void ezAnimGraph::SendResultTo(ezGameObject* pObject)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  Finalize(pSkeleton.GetPointer());

  ezMsgAnimationPoseUpdated msg;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_ModelSpaceTransforms;

  pObject->SendMessageRecursive(msg);
}

ezResult ezAnimGraph::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    stream << node->GetDynamicRTTI()->GetTypeName();

    node->SerializeNode(stream);
  }

  stream << m_hSkeleton;

  EZ_SUCCEED_OR_RETURN(m_Blackboard.Serialize(stream));

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_TriggerInputPinStates));

  stream << m_TriggerOutputToInputPinMapping.GetCount();
  for (const auto& ar : m_TriggerOutputToInputPinMapping)
  {
    EZ_SUCCEED_OR_RETURN(stream.WriteArray(ar));
  }

  return EZ_SUCCESS;
}

ezResult ezAnimGraph::Deserialize(ezStreamReader& stream)
{
  const auto uiVersion = stream.ReadVersion(2);

  ezUInt32 uiNumNodes = 0;
  stream >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  ezStringBuilder sTypeName;

  for (auto& node : m_Nodes)
  {
    stream >> sTypeName;
    node = std::move(ezRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<ezAnimGraphNode>());

    node->DeserializeNode(stream);
  }

  stream >> m_hSkeleton;

  EZ_SUCCEED_OR_RETURN(m_Blackboard.Deserialize(stream));

  if (uiVersion >= 2)
  {
    EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_TriggerInputPinStates));

    ezUInt32 sar = 0;
    stream >> sar;
    m_TriggerOutputToInputPinMapping.SetCount(sar);
    for (auto& ar : m_TriggerOutputToInputPinMapping)
    {
      EZ_SUCCEED_OR_RETURN(stream.ReadArray(ar));
    }
  }

  return EZ_SUCCESS;
}

void ezAnimGraph::AddFrameBlendLayer(const ozz::animation::BlendingJob::Layer& layer)
{
  m_ozzBlendLayers.PushBack(layer);
}

void ezAnimGraph::AddFrameRootMotion(const ezVec3& motion)
{
  m_vRootMotion += motion;
}

ezAnimGraphBlendWeights* ezAnimGraph::AllocateBlendWeights(const ezSkeletonResource& skeleton)
{
  ezAnimGraphBlendWeights* pWeights = nullptr;

  if (!m_BlendWeightsFreeList.IsEmpty())
  {
    pWeights = m_BlendWeightsFreeList.PeekBack();
    m_BlendWeightsFreeList.PopBack();
  }
  else
  {
    pWeights = &m_BlendWeights.ExpandAndGetRef();
  }

  pWeights->m_ozzBlendWeights.resize(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

  return pWeights;
}

void ezAnimGraph::FreeBlendWeights(ezAnimGraphBlendWeights*& pWeights)
{
  if (pWeights == nullptr)
    return;

  m_BlendWeightsFreeList.PushBack(pWeights);
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

  pCache->m_ozzSamplingCache.Resize(animclip.num_tracks());

  return pCache;
}

void ezAnimGraph::FreeSamplingCache(ezAnimGraphSamplingCache*& pCache)
{
  if (pCache == nullptr)
    return;

  pCache->m_ozzSamplingCache.Invalidate();
  m_SamplingCachesFreeList.PushBack(pCache);
  pCache = nullptr;
}
