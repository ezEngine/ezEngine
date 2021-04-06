#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LerpClipsAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLerpClipsAnimNode, 1, ezRTTIDefaultAllocator<ezLerpClipsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip0", GetAnimationClip0, SetAnimationClip0)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip1", GetAnimationClip1, SetAnimationClip1)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip2", GetAnimationClip2, SetAnimationClip2)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip3", GetAnimationClip3, SetAnimationClip3)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("AnimRamp", m_AnimRamp),
    EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Lerp", m_LerpPin)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OnFinished", m_OnFinishedPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation Sampling"),
    new ezColorAttribute(ezColor::RoyalBlue),
    new ezTitleAttribute("Lerp: '{AnimationClip0}' '{AnimationClip1}' '{AnimationClip2}' '{AnimationClip3}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLerpClipsAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Serialize(stream));
  stream << m_NormalizedPlaybackTime;
  stream << m_hAnimationClips[0];
  stream << m_hAnimationClips[1];
  stream << m_hAnimationClips[2];
  stream << m_hAnimationClips[3];
  stream << m_fPlaybackSpeed;
  stream << m_bApplyRootMotion;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LerpPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLerpClipsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Deserialize(stream));
  stream >> m_NormalizedPlaybackTime;
  stream >> m_hAnimationClips[0];
  stream >> m_hAnimationClips[1];
  stream >> m_hAnimationClips[2];
  stream >> m_hAnimationClips[3];
  stream >> m_fPlaybackSpeed;
  stream >> m_bApplyRootMotion;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LerpPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLerpClipsAnimNode::SetAnimationClip0(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[0] = hResource;
}

const char* ezLerpClipsAnimNode::GetAnimationClip0() const
{
  if (!m_hAnimationClips[0].IsValid())
    return "";

  return m_hAnimationClips[0].GetResourceID();
}

void ezLerpClipsAnimNode::SetAnimationClip1(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[1] = hResource;
}

const char* ezLerpClipsAnimNode::GetAnimationClip1() const
{
  if (!m_hAnimationClips[1].IsValid())
    return "";

  return m_hAnimationClips[1].GetResourceID();
}

void ezLerpClipsAnimNode::SetAnimationClip2(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[2] = hResource;
}

const char* ezLerpClipsAnimNode::GetAnimationClip2() const
{
  if (!m_hAnimationClips[2].IsValid())
    return "";

  return m_hAnimationClips[2].GetResourceID();
}

void ezLerpClipsAnimNode::SetAnimationClip3(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[3] = hResource;
}

const char* ezLerpClipsAnimNode::GetAnimationClip3() const
{
  if (!m_hAnimationClips[3].IsValid())
    return "";

  return m_hAnimationClips[3].GetResourceID();
}

void ezLerpClipsAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LerpPin.IsConnected() || !m_hAnimationClips[0].IsValid())
    return;

  ezInt32 iMaxClip = 0;
  if (m_hAnimationClips[1].IsValid())
  {
    iMaxClip = 1;

    if (m_hAnimationClips[2].IsValid())
    {
      iMaxClip = 2;

      if (m_hAnimationClips[3].IsValid())
      {
        iMaxClip = 3;
      }
    }
  }

  float fLerpFactor = ezMath::Clamp((float)m_LerpPin.GetNumber(graph), 0.0f, (float)iMaxClip);

  ezInt32 iLowerClip = ezMath::Clamp((ezInt32)ezMath::Trunc(fLerpFactor), 0, iMaxClip);
  ezInt32 iUpperClip = ezMath::Clamp(iLowerClip + 1, 0, iMaxClip);
  fLerpFactor = ezMath::Fraction(fLerpFactor);

  if (m_ActivePin.IsTriggered(graph))
  {
    m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 1.0f, tDiff);
  }
  else
  {
    m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 0.0f, tDiff);
  }

  if (m_fCurWeight <= 0.0f)
  {
    if (m_pOutputTransform)
    {
      m_OnFinishedPin.SetTriggered(graph, true);

      m_NormalizedPlaybackTime.SetZero();
      graph.FreeLocalTransforms(m_pOutputTransform);
      graph.FreeLocalTransforms(m_pLocalTransforms[0]);
      graph.FreeSamplingCache(m_pSamplingCaches[0]);
      graph.FreeLocalTransforms(m_pLocalTransforms[1]);
      graph.FreeSamplingCache(m_pSamplingCaches[1]);
    }
    return;
  }

  if (iLowerClip == iUpperClip || fLerpFactor == 0.0f || !m_hAnimationClips[iUpperClip].IsValid())
  {
    if (!m_hAnimationClips[iLowerClip].IsValid())
      return;

    // TODO: could just sample one, instead we sample the same one twice atm
    iUpperClip = iLowerClip;
  }

  ezResourceLock<ezAnimationClipResource> pAnimClipLow(m_hAnimationClips[iLowerClip], ezResourceAcquireMode::BlockTillLoaded);
  if (pAnimClipLow.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;
  ezResourceLock<ezAnimationClipResource> pAnimClipHigh(m_hAnimationClips[iUpperClip], ezResourceAcquireMode::BlockTillLoaded);
  if (pAnimClipHigh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const auto& animDescLow = pAnimClipLow->GetDescriptor();
  const ozz::animation::Animation* pOzzAnimLow = &animDescLow.GetMappedOzzAnimation(*pSkeleton);
  const auto& animDescHigh = pAnimClipHigh->GetDescriptor();
  const ozz::animation::Animation* pOzzAnimHigh = &animDescHigh.GetMappedOzzAnimation(*pSkeleton);

  if (m_pOutputTransform == nullptr)
  {
    m_pOutputTransform = graph.AllocateLocalTransforms(*pSkeleton);
    m_pLocalTransforms[0] = graph.AllocateLocalTransforms(*pSkeleton);
    m_pLocalTransforms[1] = graph.AllocateLocalTransforms(*pSkeleton);
    m_pSamplingCaches[0] = graph.AllocateSamplingCache(*pOzzAnimLow);
    m_pSamplingCaches[1] = graph.AllocateSamplingCache(*pOzzAnimHigh);
  }

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const ezTime avgDuration = ezMath::Lerp(animDescLow.GetDuration(), animDescHigh.GetDuration(), fLerpFactor);
  const ezTime fNormalizedStep = tDiff / avgDuration.GetSeconds();

  float fSpeed = m_fPlaybackSpeed;

  if (m_SpeedPin.IsConnected())
  {
    fSpeed *= (float)m_SpeedPin.GetNumber(graph);
  }

  m_NormalizedPlaybackTime += fNormalizedStep * fSpeed;
  if (fSpeed >= 0)
  {
    while (m_NormalizedPlaybackTime > ezTime::Seconds(1.0))
    {
      m_NormalizedPlaybackTime -= ezTime::Seconds(1.0);
    }
  }
  else
  {
    while (m_NormalizedPlaybackTime < ezTime::Zero())
    {
      m_NormalizedPlaybackTime += ezTime::Seconds(1.0);
    }
  }

  // sample lower anim
  {
    ozz::animation::SamplingJob job;
    job.animation = pOzzAnimLow;
    job.cache = &m_pSamplingCaches[0]->m_ozzSamplingCache;
    job.ratio = m_NormalizedPlaybackTime.AsFloatInSeconds();
    job.output = make_span(m_pLocalTransforms[0]->m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  // sample upper anim
  {
    ozz::animation::SamplingJob job;
    job.animation = pOzzAnimHigh;
    job.cache = &m_pSamplingCaches[1]->m_ozzSamplingCache;
    job.ratio = m_NormalizedPlaybackTime.AsFloatInSeconds();
    job.output = make_span(m_pLocalTransforms[1]->m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  // blend both animations
  {
    ozz::animation::BlendingJob::Layer bl[2];

    bl[0].transform = make_span(m_pLocalTransforms[0]->m_ozzLocalTransforms);
    bl[1].transform = make_span(m_pLocalTransforms[1]->m_ozzLocalTransforms);

    bl[0].weight = 1.0f - fLerpFactor;
    bl[1].weight = fLerpFactor;

    if (m_WeightsPin.IsConnected())
    {
      bl[0].joint_weights = make_span(m_WeightsPin.GetWeights(graph)->m_ozzBoneWeights);
      bl[1].joint_weights = make_span(m_WeightsPin.GetWeights(graph)->m_ozzBoneWeights);
    }

    ozz::animation::BlendingJob job;
    job.threshold = 0.1f;
    job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(bl, bl + 2);
    job.bind_pose = pOzzSkeleton->joint_bind_poses();
    job.output = make_span(m_pOutputTransform->m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  // send to output
  {
    m_pOutputTransform->m_fOverallWeight = m_fCurWeight;
    m_pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);

    m_pOutputTransform->m_bUseRootMotion = m_bApplyRootMotion;

    if (m_bApplyRootMotion)
    {
      m_pOutputTransform->m_vRootMotion = ezMath::Lerp(animDescLow.m_vConstantRootMotion, animDescHigh.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds();
    }

    m_LocalPosePin.SetPose(graph, m_pOutputTransform);
  }
}
