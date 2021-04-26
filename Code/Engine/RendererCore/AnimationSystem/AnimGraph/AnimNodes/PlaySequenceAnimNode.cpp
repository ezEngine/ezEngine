#include <RendererCorePCH.h>

#include <AnimationSystem/AnimPoseGenerator.h>
#include <AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlaySequenceAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
//
//// clang-format off
//EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlaySequenceAnimNode, 1, ezRTTIDefaultAllocator<ezPlaySequenceAnimNode>)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//    EZ_MEMBER_PROPERTY("AnimRamp", m_AnimFade),
//    EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
//    EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
//
//    EZ_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
//    EZ_MEMBER_PROPERTY("StartToMiddleCrossfade", m_StartToMiddleCrossFade),
//    EZ_ACCESSOR_PROPERTY("MiddleClip", GetMiddleClip, SetMiddleClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
//    EZ_MEMBER_PROPERTY("LoopCrossfade", m_LoopCrossFade),
//    EZ_MEMBER_PROPERTY("AllowLoopInterruption", m_bAllowLoopInterruption),
//    EZ_MEMBER_PROPERTY("MiddleToEndCrossfade", m_MiddleToEndCrossFade),
//    EZ_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
//
//    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
//    EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
//    EZ_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new ezHiddenAttribute()),
//    EZ_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new ezHiddenAttribute()),
//    EZ_MEMBER_PROPERTY("OnFinished", m_OnFinishedPin)->AddAttributes(new ezHiddenAttribute()),
//  }
//  EZ_END_PROPERTIES;
//  EZ_BEGIN_ATTRIBUTES
//  {
//    new ezCategoryAttribute("Animation Sampling"),
//    new ezColorAttribute(ezColor::RoyalBlue),
//    new ezTitleAttribute("Sequence: '{StartClip}' '{MiddleClip}' '{EndClip}'"),
//  }
//  EZ_END_ATTRIBUTES;
//}
//EZ_END_DYNAMIC_REFLECTED_TYPE;
//// clang-format on
//
//ezResult ezPlaySequenceAnimNode::SerializeNode(ezStreamWriter& stream) const
//{
//  stream.WriteVersion(1);
//
//  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));
//
//  EZ_SUCCEED_OR_RETURN(m_AnimFade.Serialize(stream));
//  stream << m_fPlaybackSpeed;
//  stream << m_bApplyRootMotion;
//  stream << m_hStartClip;
//  stream << m_hMiddleClip;
//  stream << m_hEndClip;
//  stream << m_StartToMiddleCrossFade;
//  stream << m_MiddleToEndCrossFade;
//  stream << m_LoopCrossFade;
//  stream << m_bAllowLoopInterruption;
//
//  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));
//
//  return EZ_SUCCESS;
//}
//
//ezResult ezPlaySequenceAnimNode::DeserializeNode(ezStreamReader& stream)
//{
//  stream.ReadVersion(1);
//
//  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));
//
//  EZ_SUCCEED_OR_RETURN(m_AnimFade.Deserialize(stream));
//  stream >> m_fPlaybackSpeed;
//  stream >> m_bApplyRootMotion;
//  stream >> m_hStartClip;
//  stream >> m_hMiddleClip;
//  stream >> m_hEndClip;
//  stream >> m_StartToMiddleCrossFade;
//  stream >> m_MiddleToEndCrossFade;
//  stream >> m_LoopCrossFade;
//  stream >> m_bAllowLoopInterruption;
//
//  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
//  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));
//
//  return EZ_SUCCESS;
//}
//
//void ezPlaySequenceAnimNode::SetStartClip(const char* szFile)
//{
//  ezAnimationClipResourceHandle hResource;
//
//  if (!ezStringUtils::IsNullOrEmpty(szFile))
//  {
//    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
//  }
//
//  m_hStartClip = hResource;
//}
//
//const char* ezPlaySequenceAnimNode::GetStartClip() const
//{
//  if (!m_hStartClip.IsValid())
//    return "";
//
//  return m_hStartClip.GetResourceID();
//}
//
//void ezPlaySequenceAnimNode::SetMiddleClip(const char* szFile)
//{
//  ezAnimationClipResourceHandle hResource;
//
//  if (!ezStringUtils::IsNullOrEmpty(szFile))
//  {
//    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
//  }
//
//  m_hMiddleClip = hResource;
//}
//
//const char* ezPlaySequenceAnimNode::GetMiddleClip() const
//{
//  if (!m_hMiddleClip.IsValid())
//    return "";
//
//  return m_hMiddleClip.GetResourceID();
//}
//
//void ezPlaySequenceAnimNode::SetEndClip(const char* szFile)
//{
//  ezAnimationClipResourceHandle hResource;
//
//  if (!ezStringUtils::IsNullOrEmpty(szFile))
//  {
//    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
//  }
//
//  m_hEndClip = hResource;
//}
//
//const char* ezPlaySequenceAnimNode::GetEndClip() const
//{
//  if (!m_hEndClip.IsValid())
//    return "";
//
//  return m_hEndClip.GetResourceID();
//}
//
//int CrossfadeAnimations2(ezAnimPoseGenerator& poseGen, ezAnimPoseGeneratorCommandID& out_cmdID, const ezAnimationClipResource& animClip0, const ezAnimationClipResource& animClip1, ezTime lookupTime, ezTime crossfadeDuration, ezUInt32 uiDeterministicID)
//{
//  const ezTime duration0 = animClip0.GetDescriptor().GetDuration();
//
//  if (lookupTime <= duration0 - crossfadeDuration)
//  {
//    auto& cmd = poseGen.AllocCommandSampleTrack(uiDeterministicID);
//    cmd.m_hAnimationClip = animClip0.GetResourceHandle();
//    cmd.m_SampleTime = lookupTime;
//    out_cmdID = cmd.GetCommandID();
//
//    return -1;
//  }
//
//  const ezTime lookup2 = lookupTime - (duration0 - crossfadeDuration);
//
//  if (lookupTime >= duration0)
//  {
//    auto& cmd = poseGen.AllocCommandSampleTrack(uiDeterministicID);
//    cmd.m_hAnimationClip = animClip1.GetResourceHandle();
//    cmd.m_SampleTime = lookup2;
//    out_cmdID = cmd.GetCommandID();
//
//    const ezTime duration1 = animClip1.GetDescriptor().GetDuration();
//
//    if (lookupTime >= duration0 + duration1 - crossfadeDuration)
//      return 2;
//
//    return 1;
//  }
//
//  float fLerp = (duration0 - lookupTime).AsFloatInSeconds() / crossfadeDuration.AsFloatInSeconds();
//  fLerp = 1.0f - fLerp;
//
//  // TODO: squared cross fade ?
//  //fLerp = 1.0f - ezMath::Square(fLerp);
//
//  auto& cmdCmb = poseGen.AllocCommandCombinePoses();
//  out_cmdID = cmdCmb.GetCommandID();
//
//  {
//    auto& cmd = poseGen.AllocCommandSampleTrack(uiDeterministicID);
//    cmd.m_hAnimationClip = animClip0.GetResourceHandle();
//    cmd.m_SampleTime = lookupTime;
//
//    cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
//    cmdCmb.m_InputWeights.PushBack(1.0f - fLerp);
//  }
//
//  {
//    auto& cmd = poseGen.AllocCommandSampleTrack(uiDeterministicID + 1);
//    cmd.m_hAnimationClip = animClip1.GetResourceHandle();
//    cmd.m_SampleTime = lookup2;
//
//    cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
//    cmdCmb.m_InputWeights.PushBack(fLerp);
//  }
//
//  return 0;
//}
//
//void ezPlaySequenceAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
//{
//  if (!m_ActivePin.IsConnected() || !m_LocalPosePin.IsConnected() || !m_hMiddleClip.IsValid())
//    return;
//
//  const bool bContinue = m_ActivePin.IsTriggered(graph);
//
//  if (m_State == State::Off && !bContinue)
//    return;
//
//  if (!bContinue)
//  {
//    m_bStopWhenPossible = true;
//  }
//
//  //if (!m_bStopWhenPossible)
//  {
//    // TODO: need to know how long the remaining anim will run, to ramp down
//    m_AnimFade.RampWeightUpOrDown(m_fCurWeight, 1.0f, tDiff);
//  }
//  //else
//  //{
//  //  m_AnimFade.RampWeightUpOrDown(m_fCurWeight, 0.0f, tDiff);
//  //}
//
//  const float fSpeed = m_fPlaybackSpeed * static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));
//
//  m_PlaybackTime += tDiff * fSpeed;
//
//  if (m_State == State::Off && bContinue)
//  {
//    m_State = State::Start;
//    m_bStopWhenPossible = false;
//
//    m_PlaybackTime.SetZero();
//
//    m_hPlayingClips[0] = m_hStartClip.IsValid() ? m_hStartClip : m_hMiddleClip;
//    m_hPlayingClips[1] = m_hMiddleClip;
//  }
//
//  ezResourceLock<ezAnimationClipResource> pClip0(m_hPlayingClips[0], ezResourceAcquireMode::BlockTillLoaded);
//  if (pClip0.GetAcquireResult() != ezResourceAcquireResult::Final)
//    return;
//
//  ezResourceLock<ezAnimationClipResource> pClip1(m_hPlayingClips[1], ezResourceAcquireMode::BlockTillLoaded);
//  if (pClip1.GetAcquireResult() != ezResourceAcquireResult::Final)
//    return;
//
//  ezAnimGraphPinDataLocalTransforms* pLocalTransforms[2] = {};
//  ezAnimGraphPinDataLocalTransforms* pOutputTransform = nullptr;
//
//  pOutputTransform = graph.AddPinDataLocalTransforms();
//  pLocalTransforms[0] = graph.AddPinDataLocalTransforms();
//  pLocalTransforms[1] = graph.AddPinDataLocalTransforms();
//
//  const void* pThis = this;
//  const ezUInt32 uiDeterministicID = ezHashingUtils::xxHash32(&pThis, sizeof(pThis));
//
//  if (m_State == State::End)
//  {
//    ezAnimPoseGeneratorCommandID cmdID;
//    const int fadeRes = CrossfadeAnimations2(graph.GetPoseGenerator(), cmdID, *pClip0.GetPointer(), *pClip1.GetPointer(), m_PlaybackTime, m_MiddleToEndCrossFade, uiDeterministicID);
//
//    pOutputTransform->m_CommandID = cmdID;
//
//    if (fadeRes == 2) // finished them all
//    {
//      m_State = State::Off;
//
//      m_OnFinishedPin.SetTriggered(graph, true);
//
//      return;
//    }
//  }
//
//  if (m_State == State::Start || m_State == State::Middle)
//  {
//    ezAnimPoseGeneratorCommandID cmdID;
//    const int fadeRes = CrossfadeAnimations2(graph.GetPoseGenerator(), cmdID, *pClip0.GetPointer(), *pClip1.GetPointer(), m_PlaybackTime, m_State == State::Start ? m_StartToMiddleCrossFade : m_LoopCrossFade, uiDeterministicID);
//
//    pOutputTransform->m_CommandID = cmdID;
//
//    if (fadeRes == -1 && m_bStopWhenPossible) // not yet started the cross-fade
//    {
//      m_State = State::End;
//      m_hPlayingClips[1] = m_hEndClip.IsValid() ? m_hEndClip : m_hMiddleClip;
//
//      if (m_State == State::Middle && m_bAllowLoopInterruption)
//      {
//        // TODO: somehow get the cross-fade to start right away
//      }
//    }
//
//    if (fadeRes >= +1) // crossed the cross-fade region
//    {
//      m_State = State::Middle;
//      m_PlaybackTime -= pClip0->GetDescriptor().GetDuration();
//
//      m_hPlayingClips[0] = m_hPlayingClips[1];
//      m_hPlayingClips[1] = m_hMiddleClip;
//    }
//  }
//
//  // send to output
//  {
//    pOutputTransform->m_fOverallWeight = m_fCurWeight;
//    pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);
//
//    pOutputTransform->m_bUseRootMotion = m_bApplyRootMotion;
//
//    if (m_bApplyRootMotion)
//    {
//      // TODO: sequence root motion
//      pOutputTransform->m_vRootMotion.SetZero();
//      pOutputTransform->m_vRootMotion *= fSpeed * tDiff.AsFloatInSeconds();
//    }
//
//    m_LocalPosePin.SetPose(graph, pOutputTransform);
//  }
//}
