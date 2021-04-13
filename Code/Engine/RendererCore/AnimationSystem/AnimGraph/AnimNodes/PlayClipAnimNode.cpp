#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlayClipAnimNode, 1, ezRTTIDefaultAllocator<ezPlayClipAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClip, SetAnimationClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("AnimRamp", m_AnimRamp),
      EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      EZ_MEMBER_PROPERTY("Loop", m_bLoop),
      EZ_MEMBER_PROPERTY("CancelWhenInactive", m_bCancelWhenInactive),

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
      new ezTitleAttribute("Play: '{AnimationClip}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPlayClipAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Serialize(stream));
  stream << m_PlaybackTime;
  stream << m_hAnimationClip;
  stream << m_fPlaybackSpeed;
  stream << m_bApplyRootMotion;
  stream << m_bLoop;
  stream << m_bCancelWhenInactive;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezPlayClipAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_AnimRamp.Deserialize(stream));
  stream >> m_PlaybackTime;
  stream >> m_hAnimationClip;
  stream >> m_fPlaybackSpeed;
  stream >> m_bApplyRootMotion;
  stream >> m_bLoop;
  stream >> m_bCancelWhenInactive;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OnFinishedPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezPlayClipAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_hAnimationClip.IsValid() || !m_LocalPosePin.IsConnected())
    return;

  if (!m_bIsRunning)
  {
    if (m_ActivePin.IsTriggered(graph))
    {
      m_bIsRunning = true;
      m_bKeepRunning = true;
      m_fCurWeight = 0.0f;
      m_PlaybackTime.SetZero();
    }
    else
    {
      return;
    }
  }
  else
  {
    if (!m_ActivePin.IsTriggered(graph))
    {
      m_bKeepRunning = false;
    }
  }

  if (!m_bKeepRunning && m_bCancelWhenInactive)
  {
    m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 0.0f, tDiff);

    if (m_fCurWeight <= 0.0f)
    {
      AnimationFinished(graph);
      return;
    }
  }
  else
  {
    m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 1.0f, tDiff);
  }

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto& animDesc = pAnimClip->GetDescriptor();

  float fSpeed = m_fPlaybackSpeed;

  if (m_SpeedPin.IsConnected())
  {
    fSpeed *= (float)m_SpeedPin.GetNumber(graph);
  }

  m_PlaybackTime += tDiff * fSpeed;
  if (fSpeed > 0 && m_PlaybackTime > animDesc.GetDuration())
  {
    if (m_bKeepRunning && m_bLoop)
    {
      m_PlaybackTime -= animDesc.GetDuration();
    }
    else
    {
      AnimationFinished(graph);
      return;
    }
  }
  else if (fSpeed < 0 && m_PlaybackTime < -animDesc.GetDuration())
  {
    if (m_bKeepRunning && m_bLoop)
    {
      m_PlaybackTime += animDesc.GetDuration();
    }
    else
    {
      AnimationFinished(graph);
      return;
    }
  }

  ezTime tLookup = m_PlaybackTime;
  if (tLookup < ezTime::Zero())
  {
    tLookup += animDesc.GetDuration();
  }

  const ozz::animation::Animation* pOzzAnimation = &animDesc.GetMappedOzzAnimation(*pSkeleton);

  ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

  void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_hAnimationClip = m_hAnimationClip;
  cmd.m_SampleTime = tLookup;

  {
    pLocalTransforms->m_fOverallWeight = m_fCurWeight;
    pLocalTransforms->m_pWeights = m_WeightsPin.GetWeights(graph);

    pLocalTransforms->m_bUseRootMotion = m_bApplyRootMotion;

    if (m_bApplyRootMotion)
    {
      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds();
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
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

void ezPlayClipAnimNode::AnimationFinished(ezAnimGraph& graph)
{
  EZ_ASSERT_DEV(m_bIsRunning, "Invalid state");

  m_bIsRunning = false;
  m_bKeepRunning = false;
  m_fCurWeight = 0.0f;
  m_PlaybackTime.SetZero();

  m_OnFinishedPin.SetTriggered(graph, true);
}
