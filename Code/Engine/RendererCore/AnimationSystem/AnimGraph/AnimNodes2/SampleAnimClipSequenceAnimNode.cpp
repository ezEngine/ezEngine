#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipSequenceAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleAnimClipSequenceAnimNode, 2, ezRTTIDefaultAllocator<ezSampleAnimClipSequenceAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, {})),
      EZ_MEMBER_PROPERTY("Loop", m_bLoop),
      EZ_MEMBER_PROPERTY("RootMotionAmount", m_fRootMotionAmount)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 100.0f)),
      EZ_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      EZ_ARRAY_ACCESSOR_PROPERTY("MiddleClips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      EZ_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      EZ_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new ezHiddenAttribute()),

      EZ_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnMiddleStarted", m_OutOnMiddleStarted)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnEndStarted", m_OutOnEndStarted)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Pose Generation"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
      new ezTitleAttribute("Sample Sequence: '{StartClip}' '{Clip}' '{EndClip}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSampleAnimClipSequenceAnimNode::ezSampleAnimClipSequenceAnimNode() = default;
ezSampleAnimClipSequenceAnimNode::~ezSampleAnimClipSequenceAnimNode() = default;

ezResult ezSampleAnimClipSequenceAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sStartClip;
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));
  stream << m_sEndClip;
  stream << m_fRootMotionAmount;
  stream << m_bLoop;
  stream << m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnEndStarted.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSampleAnimClipSequenceAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sStartClip;
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));
  stream >> m_sEndClip;

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

  stream >> m_bLoop;
  stream >> m_fPlaybackSpeed;

  EZ_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnEndStarted.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return EZ_SUCCESS;
}

ezUInt32 ezSampleAnimClipSequenceAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* ezSampleAnimClipSequenceAnimNode::Clips_GetValue(ezUInt32 uiIndex) const
{
  return m_Clips[uiIndex];
}

void ezSampleAnimClipSequenceAnimNode::Clips_SetValue(ezUInt32 uiIndex, const char* szValue)
{
  m_Clips[uiIndex].Assign(szValue);
}

void ezSampleAnimClipSequenceAnimNode::Clips_Insert(ezUInt32 uiIndex, const char* szValue)
{
  ezHashedString s;
  s.Assign(szValue);
  m_Clips.InsertAt(uiIndex, s);
}

void ezSampleAnimClipSequenceAnimNode::Clips_Remove(ezUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}

void ezSampleAnimClipSequenceAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if (pState->m_PlaybackTime > ezTime::MakeFromHours(10))
  {
    if (!m_InStart.IsConnected())
    {
      pState->m_State = State::Start;
    }

    pState->m_PlaybackTime = ezTime::MakeZero();
  }

  if (m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = ezTime::MakeZero();
    pState->m_State = State::Start;
  }

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  // currently we only support playing clips forwards
  const float fPlaySpeed = ezMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  ezTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  ezAnimationClipResourceHandle hCurClip;
  ezTime tCurDuration;

  while (pState->m_State != State::Off)
  {
    if (pState->m_State == State::Start)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_State = State::Middle;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      ezResourceLock<ezAnimationClipResource> pAnimClip(startClip.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_State = State::Middle;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      EZ_ASSERT_DEBUG(tCurDuration >= ezTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        tPrevSamplePos = ezTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;
        pState->m_State = State::Middle;

        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        continue;
      }

      hCurClip = startClip.m_hClip;
      break;
    }

    if (pState->m_State == State::Middle)
    {
      if (m_Clips.IsEmpty())
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (!clipInfo.m_hClip.IsValid())
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      ezResourceLock<ezAnimationClipResource> pAnimClip(clipInfo.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        pState->m_State = State::End;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      EZ_ASSERT_DEBUG(tCurDuration >= ezTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        tPrevSamplePos = ezTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;

        if (bLoop)
        {
          m_OutOnMiddleStarted.SetTriggered(ref_graph);
          pState->m_State = State::Middle;

          pState->m_uiMiddleClipIdx = static_cast<ezUInt8>(m_ClipIndexPin.GetNumber(ref_graph, 0xFF));
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        else
        {
          m_OutOnEndStarted.SetTriggered(ref_graph);
          pState->m_State = State::End;
        }
        continue;
      }

      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_State == State::End)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);

      if (!endClip.m_hClip.IsValid())
      {
        pState->m_State = State::HoldMiddleFrame;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      ezResourceLock<ezAnimationClipResource> pAnimClip(endClip.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        pState->m_State = State::HoldMiddleFrame;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      EZ_ASSERT_DEBUG(tCurDuration >= ezTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnFinished.SetTriggered(ref_graph);
        pState->m_State = State::HoldEndFrame;
        continue;
      }

      hCurClip = endClip.m_hClip;
      break;
    }

    if (pState->m_State == State::HoldEndFrame)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);
      hCurClip = endClip.m_hClip;

      ezResourceLock<ezAnimationClipResource> pAnimClip(endClip.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      pState->m_PlaybackTime = tCurDuration;
      break;
    }

    if (pState->m_State == State::HoldMiddleFrame)
    {
      if (m_Clips.IsEmpty() || pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (!clipInfo.m_hClip.IsValid())
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      ezResourceLock<ezAnimationClipResource> pAnimClip(clipInfo.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        pState->m_State = State::HoldStartFrame;
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      pState->m_PlaybackTime = tCurDuration;
      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_State == State::HoldStartFrame)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        pState->m_State = State::Off;
        continue;
      }

      ezResourceLock<ezAnimationClipResource> pAnimClip(startClip.m_hClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        pState->m_State = State::Off;
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      EZ_ASSERT_DEBUG(tCurDuration >= ezTime::MakeFromMilliseconds(5), "Too short clip");

      hCurClip = startClip.m_hClip;
      pState->m_PlaybackTime = tCurDuration;
      break;
    }
  }

  if (!hCurClip.IsValid())
    return;

  const float fInvDuration = 1.0f / tCurDuration.AsFloatInSeconds();

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(ezHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  cmd.m_hAnimationClip = hCurClip;
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

      ezResourceLock<ezAnimationClipResource> pAnimClip(hCurClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    }

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void ezSampleAnimClipSequenceAnimNode::SetStartClip(const char* szClip)
{
  m_sStartClip.Assign(szClip);
}

const char* ezSampleAnimClipSequenceAnimNode::GetStartClip() const
{
  return m_sStartClip;
}

void ezSampleAnimClipSequenceAnimNode::SetEndClip(const char* szClip)
{
  m_sEndClip.Assign(szClip);
}

const char* ezSampleAnimClipSequenceAnimNode::GetEndClip() const
{
  return m_sEndClip;
}

bool ezSampleAnimClipSequenceAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezSampleAnimClipSequenceAnimNodePatch_1_2 : public ezGraphPatch
{
public:
  ezSampleAnimClipSequenceAnimNodePatch_1_2()
    : ezGraphPatch("ezSampleAnimClipSequenceAnimNode", 2)
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

ezSampleAnimClipSequenceAnimNodePatch_1_2 g_ezSampleAnimClipSequenceAnimNodePatch_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleAnimClipSequenceAnimNode);
