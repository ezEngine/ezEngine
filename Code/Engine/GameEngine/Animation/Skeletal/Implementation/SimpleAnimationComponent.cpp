#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

using namespace ozz;
using namespace ozz::animation;
using namespace ozz::math;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ENUM_MEMBER_PROPERTY("AnimationMode", ezPropertyAnimMode, m_AnimationMode),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSimpleAnimationComponent::ezSimpleAnimationComponent() = default;
ezSimpleAnimationComponent::~ezSimpleAnimationComponent() = default;

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
}

void ezSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}


void ezSimpleAnimationComponent::SetAnimationClip(const ezAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

const ezAnimationClipResourceHandle& ezSimpleAnimationComponent::GetAnimationClip() const
{
  return m_hAnimationClip;
}

void ezSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* ezSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}

void ezSimpleAnimationComponent::Update()
{
  if (!m_hSkeleton.IsValid() || !m_hAnimationClip.IsValid() || m_fSpeed == 0.0f)
    return;

  ezResourceLock<ezAnimationClipResource> pAnimation(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  if (!UpdatePlaybackTime(GetWorld()->GetClock().GetTimeDiff(), animDesc.GetDuration()))
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ozz::animation::Animation* pOzzAnimation = &animDesc.GetMappedOzzAnimation(*pSkeleton.GetPointer());
  const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const ezUInt32 uiNumSkeletonJoints = pOzzSkeleton->num_joints();
  const ezUInt32 uiNumAnimatedJoints = pOzzAnimation->num_tracks();

  if (uiNumSkeletonJoints != uiNumAnimatedJoints)
    return;

  if (m_ozzSamplingCache.max_tracks() != uiNumAnimatedJoints)
  {
    m_ozzSamplingCache.Resize(uiNumAnimatedJoints);
    m_ozzLocalTransforms.resize(pOzzSkeleton->num_soa_joints());
    m_ozzModelTransforms.resize(uiNumSkeletonJoints);
  }

  {
    ozz::animation::SamplingJob job;
    job.animation = pOzzAnimation;
    job.cache = &m_ozzSamplingCache;
    job.ratio = m_PlaybackTime.AsFloatInSeconds() / pOzzAnimation->duration();
    job.output = make_span(m_ozzLocalTransforms);
    job.Run();
  }

  {
    ozz::animation::LocalToModelJob job;
    job.input = make_span(m_ozzLocalTransforms);
    job.output = make_span(m_ozzModelTransforms);
    job.skeleton = pOzzSkeleton;
    job.Run();
  }

  // inform child nodes/components that a new pose is available
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = ezMakeArrayPtr<const ezMat4>((const ezMat4*)&m_ozzModelTransforms[0], (ezUInt32)m_ozzModelTransforms.size());

    GetOwner()->SendMessageRecursive(msg);
  }
}

bool ezSimpleAnimationComponent::UpdatePlaybackTime(ezTime tDiff, ezTime duration)
{
  if (tDiff.IsZero() || m_fSpeed == 0.0f)
    return false;

  const ezTime tPrev = m_PlaybackTime;

  switch (m_AnimationMode)
  {
    case ezPropertyAnimMode::Once:
    {
      m_PlaybackTime += tDiff * m_fSpeed;
      m_PlaybackTime = ezMath::Clamp(m_PlaybackTime, ezTime::Zero(), duration);
      break;
    }

    case ezPropertyAnimMode::Loop:
    {
      m_PlaybackTime += tDiff * m_fSpeed;

      if (m_PlaybackTime < ezTime::Zero())
        m_PlaybackTime += duration;

      if (m_PlaybackTime > duration)
        m_PlaybackTime -= duration;

      break;
    }

    case ezPropertyAnimMode::BackAndForth:
    {
      const bool bReverse = GetUserFlag(0);

      if (bReverse)
        m_PlaybackTime -= tDiff * m_fSpeed;
      else
        m_PlaybackTime += tDiff * m_fSpeed;

      if (m_PlaybackTime > duration)
      {
        SetUserFlag(0, !bReverse);

        const ezTime tOver = duration - m_PlaybackTime;

        m_PlaybackTime = duration - tOver;
      }
      else if (m_PlaybackTime < ezTime::Zero())
      {
        SetUserFlag(0, !bReverse);

        m_PlaybackTime = -m_PlaybackTime;
      }

      break;
    }
  }

  return tPrev != m_PlaybackTime;
}
