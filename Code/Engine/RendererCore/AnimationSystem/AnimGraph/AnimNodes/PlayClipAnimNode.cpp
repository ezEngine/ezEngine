#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlayClipAnimNode, 1, ezRTTIDefaultAllocator<ezPlayClipAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClip, SetAnimationClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
      EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("RampUpTime", m_RampUp),
      EZ_MEMBER_PROPERTY("RampDownTime", m_RampDown),

      EZ_MEMBER_PROPERTY("Active", m_Active)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("SpeedPin", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Weights", m_Weights)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlayClipAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_RampUp;
  stream << m_RampDown;
  stream << m_PlaybackTime;
  stream << m_hAnimationClip;
  stream << m_fSpeed;

  EZ_SUCCEED_OR_RETURN(m_Active.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Weights.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlayClipAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_RampUp;
  stream >> m_RampDown;
  stream >> m_PlaybackTime;
  stream >> m_hAnimationClip;
  stream >> m_fSpeed;

  EZ_SUCCEED_OR_RETURN(m_Active.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Weights.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezPlayClipAnimNode::Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  if (!m_hAnimationClip.IsValid())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  float fValue = 0.0f;

  if (m_Active.IsTriggered(*pOwner))
  {
    fValue = 1.0f;
  }

  if (m_fCurWeight < fValue)
  {
    m_bIsRampingUpOrDown = true;
    m_fCurWeight += tDiff.AsFloatInSeconds() / m_RampUp.AsFloatInSeconds();
    m_fCurWeight = ezMath::Min(m_fCurWeight, fValue);
  }
  else if (m_fCurWeight > fValue)
  {
    m_bIsRampingUpOrDown = true;
    m_fCurWeight -= tDiff.AsFloatInSeconds() / m_RampDown.AsFloatInSeconds();
    m_fCurWeight = ezMath::Max(0.0f, m_fCurWeight);
  }
  else
  {
    m_bIsRampingUpOrDown = false;
  }

  if (m_fCurWeight <= 0.0f)
  {
    m_PlaybackTime.SetZero();
    // pOwner->FreeBlendWeights(m_pPartialBlendingMask); // handle this ?
    pOwner->FreeLocalTransforms(m_pLocalTransforms);
    pOwner->FreeSamplingCache(m_pSamplingCache);
    return;
  }

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const auto& animDesc = pAnimClip->GetDescriptor();

  float fSpeed = m_fSpeed;

  if (m_SpeedPin.IsConnected())
  {
    fSpeed *= (float)m_SpeedPin.GetNumber(*pOwner);
  }

  m_PlaybackTime += tDiff * fSpeed;
  if (fSpeed > 0 && m_PlaybackTime > animDesc.GetDuration())
  {
    m_PlaybackTime -= animDesc.GetDuration();
  }
  else if (fSpeed < 0 && m_PlaybackTime < ezTime::Zero())
  {
    m_PlaybackTime += animDesc.GetDuration();
  }

  const ozz::animation::Animation* pOzzAnimation = &animDesc.GetMappedOzzAnimation(*pSkeleton);

  if (m_pLocalTransforms == nullptr)
  {
    m_pLocalTransforms = pOwner->AllocateLocalTransforms(*pSkeleton);
  }

  if (m_pSamplingCache == nullptr)
  {
    m_pSamplingCache = pOwner->AllocateSamplingCache(*pOzzAnimation);
  }

  {
    ozz::animation::SamplingJob job;
    job.animation = pOzzAnimation;
    job.cache = &m_pSamplingCache->m_ozzSamplingCache;
    job.ratio = m_PlaybackTime.AsFloatInSeconds() / animDesc.GetDuration().AsFloatInSeconds();
    job.output = make_span(m_pLocalTransforms->m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  pOwner->AddFrameRootMotion(pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_fCurWeight);

  ozz::animation::BlendingJob::Layer layer;
  layer.weight = m_fCurWeight;
  layer.transform = make_span(m_pLocalTransforms->m_ozzLocalTransforms);

  if (const ezAnimGraphBlendWeights* pWeights = m_Weights.GetWeights(*pOwner))
  {
    layer.weight *= pWeights->m_fOverallWeight;
    layer.joint_weights = make_span(pWeights->m_ozzBlendWeights);
  }

  pOwner->AddFrameBlendLayer(layer);
}

void ezPlayClipAnimNode::SetAnimationClip(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClip = hResource;
}

const char* ezPlayClipAnimNode::GetAnimationClip() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}
