#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlaySequenceAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlaySequenceAnimNode, 1, ezRTTIDefaultAllocator<ezPlaySequenceAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("AnimRamp", m_AnimRamp),
    EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),

    EZ_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_MEMBER_PROPERTY("StartToMiddleCrossfade", m_StartToMiddleCrossFade),
    EZ_ACCESSOR_PROPERTY("MiddleClip", GetMiddleClip, SetMiddleClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_MEMBER_PROPERTY("LoopCrossfade", m_LoopCrossFade),
    EZ_MEMBER_PROPERTY("AllowLoopInterruption", m_bAllowLoopInterruption),
    EZ_MEMBER_PROPERTY("MiddleToEndCrossfade", m_MiddleToEndCrossFade),
    EZ_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OnFinished", m_OnFinishedPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation Sampling"),
    new ezColorAttribute(ezColor::RoyalBlue),
    new ezTitleAttribute("Sequence: '{StartClip}' '{MiddleClip}' '{EndClip}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlaySequenceAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Serialize(stream));
  stream << m_fPlaybackSpeed;
  stream << m_bApplyRootMotion;
  stream << m_hStartClip;
  stream << m_hMiddleClip;
  stream << m_hEndClip;
  stream << m_StartToMiddleCrossFade;
  stream << m_MiddleToEndCrossFade;
  stream << m_LoopCrossFade;
  stream << m_bAllowLoopInterruption;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlaySequenceAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Deserialize(stream));
  stream >> m_fPlaybackSpeed;
  stream >> m_bApplyRootMotion;
  stream >> m_hStartClip;
  stream >> m_hMiddleClip;
  stream >> m_hEndClip;
  stream >> m_StartToMiddleCrossFade;
  stream >> m_MiddleToEndCrossFade;
  stream >> m_LoopCrossFade;
  stream >> m_bAllowLoopInterruption;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));

  return EZ_SUCCESS;
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

void ezPlaySequenceAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_ActivePin.IsConnected() || !m_LocalPosePin.IsConnected() || !m_hMiddleClip.IsValid())
    return;

  const bool bContinue = m_ActivePin.IsTriggered(graph);

  if (m_State == State::Off && !bContinue)
    return;

  if (!bContinue)
  {
    m_bStopWhenPossible = true;
  }

  //if (!m_bStopWhenPossible)
  {
    // TODO: need to know how long the remaining anim will run, to ramp down
    m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 1.0f, tDiff);
  }
  //else
  //{
  //  m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 0.0f, tDiff);
  //}


  m_PlaybackTime += tDiff * m_fPlaybackSpeed;

  if (m_State == State::Off && bContinue)
  {
    m_State = State::Start;
    m_bStopWhenPossible = false;

    m_PlaybackTime.SetZero();

    m_hPlayingClips[0] = m_hStartClip.IsValid() ? m_hStartClip : m_hMiddleClip;
    m_hPlayingClips[1] = m_hMiddleClip;
  }

  ezResourceLock<ezAnimationClipResource> pClip0(m_hPlayingClips[0], ezResourceAcquireMode::BlockTillLoaded);
  if (pClip0.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezResourceLock<ezAnimationClipResource> pClip1(m_hPlayingClips[1], ezResourceAcquireMode::BlockTillLoaded);
  if (pClip1.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  if (m_pOutputTransform == nullptr)
  {
    m_pOutputTransform = graph.AllocateLocalTransforms(*pSkeleton);
    m_pLocalTransforms[0] = graph.AllocateLocalTransforms(*pSkeleton);
    m_pLocalTransforms[1] = graph.AllocateLocalTransforms(*pSkeleton);
  }

  graph.UpdateSamplingCache(m_pSamplingCache[0], pClip0->GetDescriptor().GetMappedOzzAnimation(*pSkeleton));
  graph.UpdateSamplingCache(m_pSamplingCache[1], pClip1->GetDescriptor().GetMappedOzzAnimation(*pSkeleton));

  if (m_State == State::End)
  {
    const int fadeRes = CrossfadeAnimations(*m_pOutputTransform,
      *m_pSamplingCache[0], *pClip0.GetPointer(), *m_pLocalTransforms[0],
      *m_pSamplingCache[1], *pClip1.GetPointer(), *m_pLocalTransforms[1],
      *pSkeleton, m_PlaybackTime, m_MiddleToEndCrossFade);

    if (fadeRes == 2) // finished them all
    {
      m_State = State::Off;

      graph.FreeLocalTransforms(m_pOutputTransform);
      graph.FreeLocalTransforms(m_pLocalTransforms[0]);
      graph.FreeLocalTransforms(m_pLocalTransforms[1]);
      graph.FreeSamplingCache(m_pSamplingCache[0]);
      graph.FreeSamplingCache(m_pSamplingCache[1]);

      m_OnFinishedPin.SetTriggered(graph, true);

      return;
    }
  }

  if (m_State == State::Start || m_State == State::Middle)
  {
    const int fadeRes = CrossfadeAnimations(*m_pOutputTransform,
      *m_pSamplingCache[0], *pClip0.GetPointer(), *m_pLocalTransforms[0],
      *m_pSamplingCache[1], *pClip1.GetPointer(), *m_pLocalTransforms[1],
      *pSkeleton, m_PlaybackTime, m_State == State::Start ? m_StartToMiddleCrossFade : m_LoopCrossFade);

    if (fadeRes == -1 && m_bStopWhenPossible) // not yet started the cross-fade
    {
      m_State = State::End;
      m_hPlayingClips[1] = m_hEndClip.IsValid() ? m_hEndClip : m_hMiddleClip;

      if (m_State == State::Middle && m_bAllowLoopInterruption)
      {
        // TODO: somehow get the cross-fade to start right away
      }
    }

    if (fadeRes >= +1) // crossed the cross-fade region
    {
      m_State = State::Middle;
      m_PlaybackTime -= pClip0->GetDescriptor().GetDuration();

      m_hPlayingClips[0] = m_hPlayingClips[1];
      m_hPlayingClips[1] = m_hMiddleClip;
    }
  }

  // send to output
  {
    m_pOutputTransform->m_fOverallWeight = m_fCurWeight;
    m_pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);

    m_pOutputTransform->m_bUseRootMotion = m_bApplyRootMotion;

    if (m_bApplyRootMotion)
    {
      // TODO: sequence root motion
      m_pOutputTransform->m_vRootMotion.SetZero();
    }

    m_LocalPosePin.SetPose(graph, m_pOutputTransform);
  }
}
