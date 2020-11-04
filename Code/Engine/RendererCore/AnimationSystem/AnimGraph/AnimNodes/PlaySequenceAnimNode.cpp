#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlaySequenceAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlaySequenceAnimNode, 1, ezRTTIDefaultAllocator<ezPlaySequenceAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
      EZ_ACCESSOR_PROPERTY("MiddleClip", GetMiddleClip, SetMiddleClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
      EZ_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
      EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),

      EZ_MEMBER_PROPERTY("Active", m_Active)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlaySequenceAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fSpeed;
  stream << m_hStartClip;
  stream << m_hMiddleClip;
  stream << m_hEndClip;

  EZ_SUCCEED_OR_RETURN(m_Active.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlaySequenceAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fSpeed;
  stream >> m_hStartClip;
  stream >> m_hMiddleClip;
  stream >> m_hEndClip;

  EZ_SUCCEED_OR_RETURN(m_Active.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezPlaySequenceAnimNode::Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  const bool bContinue = m_Active.IsTriggered(*pOwner);
  m_PlaybackTime += tDiff * m_fSpeed;

  if (bContinue)
  {
    if (m_State == State::Off)
    {
      m_State = State::Start;
      m_PlaybackTime.SetZero();
    }
  }
  else
  {
    if (m_State == State::Off)
    {
      pOwner->FreeLocalTransforms(m_pLocalTransforms);
      pOwner->FreeSamplingCache(m_pSamplingCache);
      return;
    }
  }

  const ezAnimationClipResource* pClipToSample = nullptr;

  if (m_State == State::Start)
  {
    // if anything fails, enter the next state
    m_State = bContinue ? State::Loop : State::End;

    if (m_hStartClip.IsValid())
    {
      ezResourceLock<ezAnimationClipResource> pAnimClip(m_hStartClip, ezResourceAcquireMode::BlockTillLoaded);
      if (pAnimClip.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        const auto& animDesc = pAnimClip->GetDescriptor();
        if (m_PlaybackTime <= animDesc.GetDuration())
        {
          // stay in the start state
          m_State = State::Start;
          pClipToSample = pAnimClip.GetPointer();
        }
        else
        {
          if (!bContinue || m_hMiddleClip.IsValid())
          {
            // continue with the next state
            m_PlaybackTime -= animDesc.GetDuration();
          }
          else
          {
            // if there is no middle / loop state, stay at the last frame of the start state
            m_State = State::Start;
            pClipToSample = pAnimClip.GetPointer();
            m_PlaybackTime = animDesc.GetDuration();
          }
        }
      }
    }
  }

  if (m_State == State::Loop)
  {
    // if anything fails, enter the next state
    m_State = State::End;

    if (m_hMiddleClip.IsValid())
    {
      ezResourceLock<ezAnimationClipResource> pAnimClip(m_hMiddleClip, ezResourceAcquireMode::BlockTillLoaded);
      if (pAnimClip.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        const auto& animDesc = pAnimClip->GetDescriptor();

        if (m_PlaybackTime > animDesc.GetDuration())
        {
          m_PlaybackTime -= animDesc.GetDuration();

          if (bContinue)
          {
            pClipToSample = pAnimClip.GetPointer();
            m_State = State::Loop;
          }
        }
        else
        {
          pClipToSample = pAnimClip.GetPointer();
          m_State = State::Loop;
        }
      }
    }
  }

  if (m_State == State::End)
  {
    // if anything fails, enter the next state
    m_State = State::Off;

    if (m_hEndClip.IsValid())
    {
      ezResourceLock<ezAnimationClipResource> pAnimClip(m_hEndClip, ezResourceAcquireMode::BlockTillLoaded);
      if (pAnimClip.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        const auto& animDesc = pAnimClip->GetDescriptor();
        if (m_PlaybackTime <= animDesc.GetDuration())
        {
          // stay in this state
          m_State = State::End;

          pClipToSample = pAnimClip.GetPointer();
        }
      }
    }
  }

  if (m_State == State::Off)
    return;

  const auto& animDesc = pClipToSample->GetDescriptor();
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

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

  pOwner->AddFrameRootMotion(animDesc.m_vConstantRootMotion * tDiff.AsFloatInSeconds());

  ozz::animation::BlendingJob::Layer layer;
  layer.weight = 1.0f;
  layer.transform = make_span(m_pLocalTransforms->m_ozzLocalTransforms);

  pOwner->AddFrameBlendLayer(layer);
}

void ezPlaySequenceAnimNode::SetStartClip(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hStartClip = hResource;
}

const char* ezPlaySequenceAnimNode::GetStartClip() const
{
  if (!m_hStartClip.IsValid())
    return "";

  return m_hStartClip.GetResourceID();
}

void ezPlaySequenceAnimNode::SetMiddleClip(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hMiddleClip = hResource;
}

const char* ezPlaySequenceAnimNode::GetMiddleClip() const
{
  if (!m_hMiddleClip.IsValid())
    return "";

  return m_hMiddleClip.GetResourceID();
}

void ezPlaySequenceAnimNode::SetEndClip(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hEndClip = hResource;
}

const char* ezPlaySequenceAnimNode::GetEndClip() const
{
  if (!m_hEndClip.IsValid())
    return "";

  return m_hEndClip.GetResourceID();
}
