#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleAnimClipAnimNode, 1, ezRTTIDefaultAllocator<ezSampleAnimClipAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, {})),
      EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      EZ_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

      EZ_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation Sampling"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("Sample Clip: '{Clip}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSampleAnimClipAnimNode::ezSampleAnimClipAnimNode() = default;
ezSampleAnimClipAnimNode::~ezSampleAnimClipAnimNode() = default;

ezResult ezSampleAnimClipAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_hClip;
  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSampleAnimClipAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_hClip;
  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSampleAnimClipAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_hClip.IsValid() || !m_OutPose.IsConnected())
    return;

  InstanceState* pState = graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(graph))
  {
    pState->m_PlaybackTime = ezTime::Zero();
    pState->m_bPlaying = true;

    m_OutOnStarted.SetTriggered(graph);
  }

  if (!pState->m_bPlaying)
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezTime tDuration = pAnimClip->GetDescriptor().GetDuration();
  const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = ezMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(graph, m_fPlaybackSpeed)));

  const ezTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  const bool bLoop = m_InLoop.GetBool(graph, m_bLoop);

  const void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  if (bLoop && pState->m_PlaybackTime > tDuration)
  {
    pState->m_PlaybackTime -= tDuration;
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
    m_OutOnStarted.SetTriggered(graph);
  }

  cmd.m_hAnimationClip = m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = ezMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos = ezMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = m_bApplyRootMotion;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(graph, pLocalTransforms);
  }

  if (cmd.m_fNormalizedSamplePos >= 1.0f && !bLoop)
  {
    m_OutOnFinished.SetTriggered(graph);
    pState->m_bPlaying = false;
  }
}

void ezSampleAnimClipAnimNode::SetClip(const char* szClip)
{
  ezAnimationClipResourceHandle hClip;

  if (!ezStringUtils::IsNullOrEmpty(szClip))
  {
    hClip = ezResourceManager::LoadResource<ezAnimationClipResource>(szClip);
  }

  m_hClip = hClip;
}

const char* ezSampleAnimClipAnimNode::GetClip() const
{
  if (!m_hClip.IsValid())
    return "";

  return m_hClip.GetResourceID();
}

bool ezSampleAnimClipAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
