#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MixClips1DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMixClips1DAnimNode, 1, ezRTTIDefaultAllocator<ezMixClips1DAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Common", m_State),
    EZ_ACCESSOR_PROPERTY("AnimationClip0", GetAnimationClip0, SetAnimationClip0)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip1", GetAnimationClip1, SetAnimationClip1)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip2", GetAnimationClip2, SetAnimationClip2)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimationClip3", GetAnimationClip3, SetAnimationClip3)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),

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
    new ezColorAttribute(ezColor::SteelBlue),
    new ezTitleAttribute("Mix1D: '{AnimationClip0}' '{AnimationClip1}' '{AnimationClip2}' '{AnimationClip3}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezMixClips1DAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  stream << m_hAnimationClips[0];
  stream << m_hAnimationClips[1];
  stream << m_hAnimationClips[2];
  stream << m_hAnimationClips[3];

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LerpPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezMixClips1DAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  stream >> m_hAnimationClips[0];
  stream >> m_hAnimationClips[1];
  stream >> m_hAnimationClips[2];
  stream >> m_hAnimationClips[3];

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LerpPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezMixClips1DAnimNode::SetAnimationClip0(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[0] = hResource;
}

const char* ezMixClips1DAnimNode::GetAnimationClip0() const
{
  if (!m_hAnimationClips[0].IsValid())
    return "";

  return m_hAnimationClips[0].GetResourceID();
}

void ezMixClips1DAnimNode::SetAnimationClip1(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[1] = hResource;
}

const char* ezMixClips1DAnimNode::GetAnimationClip1() const
{
  if (!m_hAnimationClips[1].IsValid())
    return "";

  return m_hAnimationClips[1].GetResourceID();
}

void ezMixClips1DAnimNode::SetAnimationClip2(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[2] = hResource;
}

const char* ezMixClips1DAnimNode::GetAnimationClip2() const
{
  if (!m_hAnimationClips[2].IsValid())
    return "";

  return m_hAnimationClips[2].GetResourceID();
}

void ezMixClips1DAnimNode::SetAnimationClip3(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClips[3] = hResource;
}

const char* ezMixClips1DAnimNode::GetAnimationClip3() const
{
  if (!m_hAnimationClips[3].IsValid())
    return "";

  return m_hAnimationClips[3].GetResourceID();
}

void ezMixClips1DAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LerpPin.IsConnected() || !m_hAnimationClips[0].IsValid())
    return;

  if (m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
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

  if (iLowerClip == iUpperClip || fLerpFactor == 0.0f || !m_hAnimationClips[iUpperClip].IsValid())
  {
    if (!m_hAnimationClips[iLowerClip].IsValid())
      return;

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

  ezAnimGraphPinDataLocalTransforms* pLocalTransforms[2] = {};
  ezAnimGraphPinDataLocalTransforms* pOutputTransform = nullptr;

  pOutputTransform = graph.AddPinDataLocalTransforms();
  pLocalTransforms[0] = graph.AddPinDataLocalTransforms();
  pLocalTransforms[1] = graph.AddPinDataLocalTransforms();

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const ezTime avgDuration = ezMath::Lerp(animDescLow.GetDuration(), animDescHigh.GetDuration(), fLerpFactor);
  const float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive = m_ActivePin.IsTriggered(graph);
  m_State.m_Duration = avgDuration;
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  m_State.UpdateState(tDiff);

  if (m_State.GetCurrentState() == ezAnimState::State::StartedRampDown)
  {
    m_OnFinishedPin.SetTriggered(graph, true);
  }

  if (m_State.GetWeight() <= 0.0f)
    return;

  ezAnimPoseEventTrackSampleMode eventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  if (m_State.HasLoopedStart())
    eventSampling = ezAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    eventSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;

  auto& poseGen = graph.GetPoseGenerator();

  if (m_hAnimationClips[iLowerClip] == m_hAnimationClips[iUpperClip])
  {
    void* pThis = this;
    auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip = m_hAnimationClips[iLowerClip];
    cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
    cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
    cmd.m_EventSampling = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb = graph.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample lower anim
    {
      void* pThis = this;
      auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip = m_hAnimationClips[iLowerClip];
      cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling = fLerpFactor <= 0.5f ? eventSampling : ezAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample upper anim
    {
      void* pThis = this;
      auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip = m_hAnimationClips[iUpperClip];
      cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling = fLerpFactor > 0.5f ? eventSampling : ezAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    pOutputTransform->m_fOverallWeight = m_State.GetWeight();
    pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = ezMath::Lerp(animDescLow.m_vConstantRootMotion, animDescHigh.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    m_LocalPosePin.SetPose(graph, pOutputTransform);
  }
}
