#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipOnceAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlayClipOnceAnimNode, 1, ezRTTIDefaultAllocator<ezPlayClipOnceAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, {})),
      EZ_ARRAY_ACCESSOR_PROPERTY("Clips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

      EZ_MEMBER_PROPERTY("Activate", m_ActivatePin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OnFinished", m_OnFinishedPin)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation Sampling"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("Play Once: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlayClipOnceAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));

  EZ_SUCCEED_OR_RETURN(m_ActivatePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlayClipOnceAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));

  EZ_SUCCEED_OR_RETURN(m_ActivatePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));

  // make sure there are no invalid clips in the clip array
  for (ezUInt32 i = m_Clips.GetCount(); i > 0; i--)
  {
    if (!m_Clips[i - 1].IsValid())
    {
      m_Clips.RemoveAtAndCopy(i - 1);
    }
  }

  return EZ_SUCCESS;
}

void ezPlayClipOnceAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (m_Clips.IsEmpty() || !m_LocalPosePin.IsConnected())
    return;

  if (m_ActivatePin.IsTriggered(graph))
  {
    // always restart playing the clip when the activate pin is triggered

    m_PlaybackTime = ezTime::Zero();

    m_uiClipToPlay = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(graph, 0xFF));

    if (m_uiClipToPlay >= m_Clips.GetCount())
    {
      m_uiClipToPlay = static_cast<ezUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount()));
    }
  }

  if (m_uiClipToPlay >= m_Clips.GetCount())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_Clips[m_uiClipToPlay], ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezTime tDuration = pAnimClip->GetDescriptor().GetDuration();
  const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = ezMath::Max(0.0f, static_cast<float>(m_SpeedPin.GetNumber(graph, m_fPlaybackSpeed)));

  const ezTime tPrevSamplePos = m_PlaybackTime;
  m_PlaybackTime += tDiff * fPlaySpeed;

  const void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));

  cmd.m_hAnimationClip = m_Clips[m_uiClipToPlay];
  cmd.m_fPreviousNormalizedSamplePos = ezMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos = ezMath::Clamp(m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  {
    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = m_WeightsPin.GetWeights(graph);
    // pLocalTransforms->m_bUseRootMotion = true;
    // pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }

  if (cmd.m_fNormalizedSamplePos >= 1.0f)
  {
    m_uiClipToPlay = 0xFF;
    m_OnFinishedPin.SetTriggered(graph);
  }
}

ezUInt32 ezPlayClipOnceAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* ezPlayClipOnceAnimNode::Clips_GetValue(ezUInt32 uiIndex) const
{
  const auto& hMat = m_Clips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void ezPlayClipOnceAnimNode::Clips_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    m_Clips[uiIndex] = ezAnimationClipResourceHandle();
  else
  {
    m_Clips[uiIndex] = ezResourceManager::LoadResource<ezAnimationClipResource>(value);
  }
}

void ezPlayClipOnceAnimNode::Clips_Insert(ezUInt32 uiIndex, const char* value)
{
  ezAnimationClipResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezAnimationClipResource>(value);

  m_Clips.Insert(hMat, uiIndex);
}

void ezPlayClipOnceAnimNode::Clips_Remove(ezUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}
