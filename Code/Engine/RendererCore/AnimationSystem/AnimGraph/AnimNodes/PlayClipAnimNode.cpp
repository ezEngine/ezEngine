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
      EZ_ACCESSOR_PROPERTY("PartialBlendingRootBone", GetPartialBlendingRootBone, SetPartialBlendingRootBone),

      EZ_MEMBER_PROPERTY("Active", m_Active)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlayClipAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_RampUp;
  stream << m_RampDown;
  stream << m_PlaybackTime;
  stream << m_hAnimationClip;
  stream << m_fSpeed;
  stream << m_sPartialBlendingRootBone;

  EZ_SUCCEED_OR_RETURN(m_Active.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlayClipAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_RampUp;
  stream >> m_RampDown;
  stream >> m_PlaybackTime;
  stream >> m_hAnimationClip;
  stream >> m_fSpeed;
  stream >> m_sPartialBlendingRootBone;

  EZ_SUCCEED_OR_RETURN(m_Active.Deserialize(stream));

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
    pOwner->FreeBlendWeights(m_pPartialBlendingMask);
    pOwner->FreeLocalTransforms(m_pLocalTransforms);
    pOwner->FreeSamplingCache(m_pSamplingCache);
    return;
  }

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const auto& animDesc = pAnimClip->GetDescriptor();

  m_PlaybackTime += tDiff * m_fSpeed;
  if (m_fSpeed > 0 && m_PlaybackTime > animDesc.GetDuration())
  {
    m_PlaybackTime -= animDesc.GetDuration();
  }
  else if (m_fSpeed < 0 && m_PlaybackTime < ezTime::Zero())
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

  if (!m_sPartialBlendingRootBone.IsEmpty())
  {
    if (m_pPartialBlendingMask == nullptr)
    {
      m_pPartialBlendingMask = pOwner->AllocateBlendWeights(*pSkeleton);

      ezMemoryUtils::ZeroFill<ezUInt8>((ezUInt8*)m_pPartialBlendingMask->m_ozzBlendWeights.data(), m_pPartialBlendingMask->m_ozzBlendWeights.size() * sizeof(ozz::math::SimdFloat4));

      int iRootBone = -1;
      for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
      {
        if (ezStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], m_sPartialBlendingRootBone.GetData()))
        {
          iRootBone = iBone;
          break;
        }
      }

      const float fBoneWeight = 10.0f * m_fCurWeight;

      auto setBoneWeight = [&](int currentBone, int) {
        const int iJointIdx0 = currentBone / 4;
        const int iJointIdx1 = currentBone % 4;

        ozz::math::SimdFloat4& soa_weight = m_pPartialBlendingMask->m_ozzBlendWeights.at(iJointIdx0);
        soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
      };

      ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
    }
  }
  else
  {
    pOwner->FreeBlendWeights(m_pPartialBlendingMask);
  }

  pOwner->AddFrameRootMotion(pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_fCurWeight);

  ozz::animation::BlendingJob::Layer layer;
  layer.weight = m_fCurWeight;
  layer.transform = make_span(m_pLocalTransforms->m_ozzLocalTransforms);

  if (m_pPartialBlendingMask != nullptr)
  {
    layer.joint_weights = make_span(m_pPartialBlendingMask->m_ozzBlendWeights);
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

void ezPlayClipAnimNode::SetPartialBlendingRootBone(const char* szBone)
{
  m_sPartialBlendingRootBone.Assign(szBone);
}

const char* ezPlayClipAnimNode::GetPartialBlendingRootBone() const
{
  return m_sPartialBlendingRootBone.GetData();
}
