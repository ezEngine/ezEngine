#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlayClipAnimNode, 1, ezRTTIDefaultAllocator<ezPlayClipAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Common", m_State),
      EZ_ARRAY_ACCESSOR_PROPERTY("Clips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

      EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation Sampling"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("Play: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlayClipAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlayClipAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  // make sure there are no invalid clips in the middle clip array
  for (ezUInt32 i = m_Clips.GetCount(); i > 0; i--)
  {
    if (!m_Clips[i - 1].IsValid())
    {
      m_Clips.RemoveAtAndSwap(i - 1);
    }
  }

  return EZ_SUCCESS;
}

void ezPlayClipAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (m_Clips.IsEmpty() || !m_LocalPosePin.IsConnected() || m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
  {
    m_uiClipToPlay = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  ezUInt8 uiNextClip = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(graph, m_uiNextClipToPlay));

  if (uiNextClip >= m_Clips.GetCount())
  {
    uiNextClip = static_cast<ezUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount()));
  }

  if (m_uiNextClipToPlay != uiNextClip)
  {
    ezResourceLock<ezAnimationClipResource> pNextClip(m_Clips[uiNextClip], ezResourceAcquireMode::BlockTillLoaded);
    if (pNextClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    m_uiNextClipToPlay = uiNextClip;
    m_NextClipDuration = pNextClip->GetDescriptor().GetDuration();
  }

  if (m_uiClipToPlay >= m_Clips.GetCount())
  {
    m_uiClipToPlay = uiNextClip;
    m_uiNextClipToPlay = 0xFF; // make sure the next update will pick another random clip
  }

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_Clips[m_uiClipToPlay], ezResourceAcquireMode::BlockTillLoaded);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive = m_ActivePin.IsTriggered(graph);
  m_State.m_Duration = pAnimClip->GetDescriptor().GetDuration();
  m_State.m_DurationOfQueued = m_State.m_bLoop ? m_NextClipDuration : ezTime::Zero();
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  m_State.UpdateState(tDiff);

  void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));

  if (m_Clips.GetCount() > 1 && m_State.HasTransitioned())
  {
    // guarantee that all animation events from the just finished first clip get evaluated and sent
    {
      auto& cmdE = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
      cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmdE.m_fNormalizedSamplePos = m_State.GetFinalSpeed() > 0 ? 1.1f : -0.1f;
      cmdE.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;
      cmdE.m_hAnimationClip = m_Clips[m_uiClipToPlay];

      cmd.m_Inputs.PushBack(cmdE.GetCommandID());
    }

    m_uiClipToPlay = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
    m_uiNextClipToPlay = 0xFF;
    m_NextClipDuration.SetZero();

    fPrevPlaybackPos = 0.0f;
  }

  if (m_State.GetCurrentState() == ezAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }

  cmd.m_hAnimationClip = m_Clips[m_uiClipToPlay];
  cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
  cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;

  if (m_State.HasLoopedStart())
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
  else
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  {
    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_fOverallWeight = m_State.GetWeight();
    pLocalTransforms->m_pWeights = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
}

ezUInt32 ezPlayClipAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* ezPlayClipAnimNode::Clips_GetValue(ezUInt32 uiIndex) const
{
  const auto& hMat = m_Clips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void ezPlayClipAnimNode::Clips_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    m_Clips[uiIndex] = ezAnimationClipResourceHandle();
  else
  {
    m_Clips[uiIndex] = ezResourceManager::LoadResource<ezAnimationClipResource>(value);
  }
}

void ezPlayClipAnimNode::Clips_Insert(ezUInt32 uiIndex, const char* value)
{
  ezAnimationClipResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezAnimationClipResource>(value);

  m_Clips.Insert(hMat, uiIndex);
}

void ezPlayClipAnimNode::Clips_Remove(ezUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlayClipAnimNode);
