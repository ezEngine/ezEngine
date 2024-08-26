#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleFrameAnimNode, 1, ezRTTIDefaultAllocator<ezSampleFrameAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      EZ_MEMBER_PROPERTY("NormPos", m_fNormalizedSamplePosition)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),

      EZ_MEMBER_PROPERTY("InNormPos", m_InNormalizedSamplePosition)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InAbsPos", m_InAbsoluteSamplePosition)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Pose Generation"),
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

  stream << m_sClip;
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

  stream >> m_sClip;
  stream >> m_fNormalizedSamplePosition;

  EZ_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSampleFrameAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const auto& clip = ref_controller.GetAnimationClipInfo(m_sClip);

  if (clip.m_hClip.IsValid())
  {
    ezResourceLock<ezAnimationClipResource> pAnimClip(clip.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    float fNormPos = static_cast<float>(m_InNormalizedSamplePosition.GetNumber(ref_graph, m_fNormalizedSamplePosition));

    if (m_InAbsoluteSamplePosition.IsConnected())
    {
      const ezTime tDuration = pAnimClip->GetDescriptor().GetDuration();
      const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();
      fNormPos = static_cast<float>(m_InAbsoluteSamplePosition.GetNumber(ref_graph) * fInvDuration);
    }

    fNormPos = ezMath::Clamp(fNormPos, 0.0f, 1.0f);

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));

    cmd.m_hAnimationClip = clip.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = fNormPos;
    cmd.m_fNormalizedSamplePos = fNormPos;
    cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;

    {
      ezAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
  else
  {
    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      ezAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
}

void ezSampleFrameAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* ezSampleFrameAnimNode::GetClip() const
{
  return m_sClip.GetData();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleFrameAnimNode);
