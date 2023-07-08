#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleBlendSpace2DAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAnimationClip2D, ezNoBase, 1, ezRTTIDefaultAllocator<ezAnimationClip2D>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    EZ_MEMBER_PROPERTY("Position", m_vPosition),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleBlendSpace2DAnimNode, 1, ezRTTIDefaultAllocator<ezSampleBlendSpace2DAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, {})),
      EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      EZ_MEMBER_PROPERTY("InputResponse", m_InputResponse)->AddAttributes(new ezDefaultValueAttribute(ezTime::Milliseconds(100))),
    EZ_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
      EZ_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      EZ_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("X", m_InCoordX)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Y", m_InCoordY)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation Sampling"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("BlendSpace 2D: '{CenterClip}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezAnimationClip2D::SetAnimationFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimation = hResource;
}

const char* ezAnimationClip2D::GetAnimationFile() const
{
  if (m_hAnimation.IsValid())
  {
    return m_hAnimation.GetResourceID();
  }

  return "";
}

ezSampleBlendSpace2DAnimNode::ezSampleBlendSpace2DAnimNode() = default;
ezSampleBlendSpace2DAnimNode::~ezSampleBlendSpace2DAnimNode() = default;

ezResult ezSampleBlendSpace2DAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_hCenterClip;

  stream << m_Clips.GetCount();
  for (ezUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_hAnimation;
    stream << m_Clips[i].m_vPosition;
  }

  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;
  stream << m_InputResponse;

  EZ_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InCoordX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InCoordY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSampleBlendSpace2DAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_hCenterClip;

  ezUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (ezUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_hAnimation;
    stream >> m_Clips[i].m_vPosition;
  }

  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;
  stream >> m_InputResponse;

  EZ_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InCoordX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InCoordY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSampleBlendSpace2DAnimNode::SetCenterClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hCenterClip = hResource;
}

const char* ezSampleBlendSpace2DAnimNode::GetCenterClipFile() const
{
  if (!m_hCenterClip.IsValid())
    return nullptr;

  return m_hCenterClip.GetResourceID();
}

void ezSampleBlendSpace2DAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || (!m_InCoordX.IsConnected() && !m_InCoordY.IsConnected()) || m_Clips.IsEmpty())
    return;

  InstanceState* pState = graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(graph))
  {
    pState->m_CenterPlaybackTime = ezTime::Zero();
    pState->m_fOtherPlaybackPosNorm = 0.0f;
    pState->m_bPlaying = true;

    m_OutOnStarted.SetTriggered(graph);
  }

  if (!pState->m_bPlaying)
    return;

  const float x = static_cast<float>(m_InCoordX.GetNumber(graph));
  const float y = static_cast<float>(m_InCoordY.GetNumber(graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    pState->m_fLastValueX = x;
    pState->m_fLastValueY = y;
  }
  else
  {
    const float lerp = static_cast<float>(ezMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    pState->m_fLastValueX = ezMath::Lerp(pState->m_fLastValueX, x, lerp);
    pState->m_fLastValueY = ezMath::Lerp(pState->m_fLastValueY, y, lerp);
  }

  ezUInt32 uiMaxWeightClip = 0;
  ezHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(ezVec2(pState->m_fLastValueX, pState->m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(pState, graph, tDiff, clips, uiMaxWeightClip);
}

void ezSampleBlendSpace2DAnimNode::ComputeClipsAndWeights(const ezVec2& p, ezDynamicArray<ClipToPlay>& clips, ezUInt32& out_uiMaxWeightClip) const
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight = -1.0f;

  if (m_Clips.GetCount() == 1 && !m_hCenterClip.IsValid())
  {
    clips.ExpandAndGetRef().m_uiIndex = 0;
  }
  else
  {
    // this algorithm is taken from http://runevision.com/thesis chapter 6.3 "Gradient Band Interpolation"
    // also see http://answers.unity.com/answers/1208837/view.html

    float fWeightNormalization = 0.0f;

    for (ezUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const ezVec2 pi = m_Clips[i].m_vPosition;
      float fMinWeight = 1.0f;

      for (ezUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const ezVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = (pi - pj).GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi - pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = ezMath::Min(fMinWeight, fWeight);
      }

      // also check against center clip
      if (m_hCenterClip.IsValid())
      {
        const float fLenSqr = pi.GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = ezMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = i;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    // also compute weight for center clip
    if (m_hCenterClip.IsValid())
    {
      float fMinWeight = 1.0f;

      for (ezUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const ezVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = pj.GetLengthSquared();
        const float fProjLenSqr = (-p).Dot(-pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = ezMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = 0xFFFFFFFF;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    fWeightNormalization = 1.0f / fWeightNormalization;

    for (ezUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      auto& c = clips[i];

      c.m_fWeight *= fWeightNormalization;

      if (c.m_fWeight > fMaxWeight)
      {
        fMaxWeight = c.m_fWeight;
        out_uiMaxWeightClip = i;
      }
    }
  }
}

void ezSampleBlendSpace2DAnimNode::PlayClips(InstanceState* pState, ezAnimGraphInstance& graph, ezTime tDiff, ezArrayPtr<ClipToPlay> clips, ezUInt32 uiMaxWeightClip) const
{
  const bool bLoop = m_InLoop.GetBool(graph, m_bLoop);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(graph, m_fPlaybackSpeed));

  ezTime tAvgDuration = ezTime::Zero();

  ezHybridArray<ezAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  ezVec3 vRootMotion = ezVec3::ZeroVector();
  ezUInt32 uiNumAvgClips = 0;

  for (ezUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const ezAnimationClipResourceHandle& hClip = c.m_uiIndex >= 0xFF ? m_hCenterClip : m_Clips[c.m_uiIndex].m_hAnimation;

    ezResourceLock<ezAnimationClipResource> pClip(hClip, ezResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    const void* pThis = this;
    auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip = hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;
    vRootMotion += pClip->GetDescriptor().m_vConstantRootMotion * c.m_fWeight;
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  tAvgDuration = ezMath::Max(tAvgDuration, ezTime::Milliseconds(16));

  const ezTime fPrevCenterPlaybackPos = pState->m_CenterPlaybackTime;
  const float fPrevPlaybackPosNorm = pState->m_fOtherPlaybackPosNorm;

  ezAnimPoseEventTrackSampleMode eventSamplingCenter = ezAnimPoseEventTrackSampleMode::OnlyBetween;
  ezAnimPoseEventTrackSampleMode eventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  const float fInvAvgDuration = 1.0f / tAvgDuration.AsFloatInSeconds();
  const float tDiffNorm = tDiff.AsFloatInSeconds() * fInvAvgDuration;

  // now that we know the duration, we can finally update the playback state
  pState->m_fOtherPlaybackPosNorm += tDiffNorm * fSpeed;
  while (pState->m_fOtherPlaybackPosNorm >= 1.0f)
  {
    if (bLoop)
    {
      pState->m_fOtherPlaybackPosNorm -= 1.0f;
      m_OutOnStarted.SetTriggered(graph);
      eventSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    else
    {
      pState->m_fOtherPlaybackPosNorm = 1.0f;
      pState->m_bPlaying = false;
      m_OutOnFinished.SetTriggered(graph);
    }
  }

  UpdateCenterClipPlaybackTime(pState, graph, tDiff, eventSamplingCenter);

  for (ezUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    if (pSampleTrack[i]->m_hAnimationClip == m_hCenterClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSamplingCenter : ezAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPosNorm;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_fOtherPlaybackPosNorm;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSampling : ezAnimPoseEventTrackSampleMode::None;
    }
  }

  ezAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();

  if (m_bApplyRootMotion)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(graph, m_fPlaybackSpeed));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed;
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb = graph.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (ezUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i] = pSampleTrack[i]->GetCommandID();
    }
  }

  m_OutPose.SetPose(graph, pOutputTransform);
}

void ezSampleBlendSpace2DAnimNode::UpdateCenterClipPlaybackTime(InstanceState* pState, ezAnimGraphInstance& graph, ezTime tDiff, ezAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const
{
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(graph, m_fPlaybackSpeed));

  if (m_hCenterClip.IsValid())
  {
    ezResourceLock<ezAnimationClipResource> pClip(m_hCenterClip, ezResourceAcquireMode::BlockTillLoaded);

    const ezTime tDur = pClip->GetDescriptor().GetDuration();

    pState->m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (pState->m_CenterPlaybackTime > tDur)
    {
      pState->m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
  }
}

bool ezSampleBlendSpace2DAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
