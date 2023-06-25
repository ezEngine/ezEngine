#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleFrameAnimNode, 1, ezRTTIDefaultAllocator<ezSampleFrameAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
      EZ_MEMBER_PROPERTY("NormPos", m_fNormalizedSamplePosition)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),

      EZ_MEMBER_PROPERTY("InNormPos", m_InNormalizedSamplePosition)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InAbsPos", m_InAbsoluteSamplePosition)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation Sampling"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("Sample Frame: '{Clip}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSampleFrameAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_hClip;
  stream << m_fNormalizedSamplePosition;

  EZ_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSampleFrameAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_hClip;
  stream >> m_fNormalizedSamplePosition;

  EZ_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSampleFrameAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_hClip.IsValid() || !m_OutPose.IsConnected())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  float fNormPos = fNormPos = m_InNormalizedSamplePosition.GetNumber(graph, m_fNormalizedSamplePosition);

  if (m_InAbsoluteSamplePosition.IsConnected())
  {
    const ezTime tDuration = pAnimClip->GetDescriptor().GetDuration();
    const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();
    fNormPos = m_InAbsoluteSamplePosition.GetNumber(graph) * fInvDuration;
  }

  fNormPos = ezMath::Clamp(fNormPos, 0.0f, 1.0f);

  const void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));

  cmd.m_hAnimationClip = m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = fNormPos;
  cmd.m_fNormalizedSamplePos = fNormPos;
  cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

  {
    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = false;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(graph, pLocalTransforms);
  }
}

void ezSampleFrameAnimNode::SetClip(const char* szClip)
{
  ezAnimationClipResourceHandle hClip;

  if (!ezStringUtils::IsNullOrEmpty(szClip))
  {
    hClip = ezResourceManager::LoadResource<ezAnimationClipResource>(szClip);
  }

  m_hClip = hClip;
}

const char* ezSampleFrameAnimNode::GetClip() const
{
  if (!m_hClip.IsValid())
    return "";

  return m_hClip.GetResourceID();
}
