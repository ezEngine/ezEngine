#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleAnimClipAnimNode, 2, ezRTTIDefaultAllocator<ezSampleAnimClipAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, {})),
      EZ_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 100.0f)),
      EZ_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),

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
      new ezCategoryAttribute("Pose Generation"),
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
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_bLoop;
  stream << m_fRootMotionAmount;
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
  const auto version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_bLoop;

  if (version == 1)
  {
    bool bApplyRootMotion = false;
    stream >> bApplyRootMotion;
    m_fRootMotionAmount = bApplyRootMotion ? 1.0f : 0.0f;
  }
  else if (version >= 2)
  {
    stream >> m_fRootMotionAmount;
  }

  stream >> m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSampleAnimClipAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_sClip);

  if (!clipInfo.m_hClip.IsValid() || !m_OutPose.IsConnected())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(clipInfo.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if (!m_InStart.IsConnected() && pState->m_PlaybackTime > ezTime::MakeFromHours(10))
  {
    pState->m_PlaybackTime = ezTime::MakeZero();
  }

  if (m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = ezTime::MakeZero();

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  const ezTime tDuration = pAnimClip->GetDescriptor().GetDuration();
  const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = ezMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  const ezTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  if (pState->m_PlaybackTime >= tDuration)
  {
    if (bLoop)
    {
      pState->m_PlaybackTime -= tDuration;
      cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
      m_OutOnStarted.SetTriggered(ref_graph);
    }
    else
    {
      if (tPrevSamplePos < tDuration)
      {
        m_OutOnFinished.SetTriggered(ref_graph);
      }
      else
      {
        // if we are already holding the last frame, we can skip event sampling
        cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::None;
      }
    }
  }

  cmd.m_hAnimationClip = clipInfo.m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = ezMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos = ezMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    ezAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    if (m_fRootMotionAmount != 0.0f)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed * m_fRootMotionAmount;
    }

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void ezSampleAnimClipAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* ezSampleAnimClipAnimNode::GetClip() const
{
  return m_sClip.GetData();
}

bool ezSampleAnimClipAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezSampleAnimClipAnimNodePatch_1_2 : public ezGraphPatch
{
public:
  ezSampleAnimClipAnimNodePatch_1_2()
    : ezGraphPatch("ezSampleAnimClipAnimNode", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    if (auto pProp = pNode->FindProperty("ApplyRootMotion"))
    {
      if (pProp->m_Value.IsA<bool>())
      {
        const bool bApply = pProp->m_Value.Get<bool>();

        if (bApply)
        {
          pNode->AddProperty("RootMotionAmount", 1.0f);
        }
      }
    }
  }
};

ezSampleAnimClipAnimNodePatch_1_2 g_ezSampleAnimClipAnimNodePatch_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleAnimClipAnimNode);
