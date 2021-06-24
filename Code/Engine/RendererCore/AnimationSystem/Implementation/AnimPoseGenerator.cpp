#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <AnimationSystem/Declarations.h>
#include <AnimationSystem/SkeletonResource.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/span.h>

void ezAnimPoseGenerator::Reset(const ezSkeletonResource* pSkeleton)
{
  m_pSkeleton = pSkeleton;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsModelPoseToOutput.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  //m_UsedModelTransforms.Clear();
  //m_SamplingCaches.Clear();
}

static EZ_ALWAYS_INLINE ezAnimPoseGeneratorCommandID CreateCommandID(ezAnimPoseGeneratorCommandType type, ezUInt32 uiIndex)
{
  return (static_cast<ezUInt32>(type) << 24u) | uiIndex;
}

static EZ_ALWAYS_INLINE ezUInt32 GetCommandIndex(ezAnimPoseGeneratorCommandID id)
{
  return static_cast<ezUInt32>(id) & 0x00FFFFFFu;
}

static EZ_ALWAYS_INLINE ezAnimPoseGeneratorCommandType GetCommandType(ezAnimPoseGeneratorCommandID id)
{
  return static_cast<ezAnimPoseGeneratorCommandType>(static_cast<ezUInt32>(id) >> 24u);
}

ezAnimPoseGeneratorCommandSampleTrack& ezAnimPoseGenerator::AllocCommandSampleTrack(ezUInt32 uiDeterministicID)
{
  auto& cmd = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.m_Type = ezAnimPoseGeneratorCommandType::SampleTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleTrack.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;
  cmd.m_uiUniqueID = uiDeterministicID;

  return cmd;
}

ezAnimPoseGeneratorCommandCombinePoses& ezAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.m_Type = ezAnimPoseGeneratorCommandType::CombinePoses;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsCombinePoses.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

ezAnimPoseGeneratorCommandLocalToModelPose& ezAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.m_Type = ezAnimPoseGeneratorCommandType::LocalToModelPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsLocalToModelPose.GetCount() - 1);
  cmd.m_ModelPoseOutput = m_ModelPoseCounter++;

  return cmd;
}

ezAnimPoseGeneratorCommandModelPoseToOutput& ezAnimPoseGenerator::AllocCommandModelPoseToOutput()
{
  auto& cmd = m_CommandsModelPoseToOutput.ExpandAndGetRef();
  cmd.m_Type = ezAnimPoseGeneratorCommandType::ModelPoseToOutput;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsModelPoseToOutput.GetCount() - 1);

  return cmd;
}

ezAnimPoseGeneratorCommandSampleEventTrack& ezAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type = ezAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

  return cmd;
}

ezAnimPoseGenerator::ezAnimPoseGenerator() = default;

ezAnimPoseGenerator::~ezAnimPoseGenerator()
{
  for (ezUInt32 i = 0; i < m_SamplingCaches.GetCount(); ++i)
  {
    EZ_DEFAULT_DELETE(m_SamplingCaches.GetValue(i));
  }
  m_SamplingCaches.Clear();
}

void ezAnimPoseGenerator::Validate() const
{
  EZ_ASSERT_DEV(m_CommandsModelPoseToOutput.GetCount() <= 1, "Only one output node may exist");

  for (auto& cmd : m_CommandsSampleTrack)
  {
    EZ_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    //EZ_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    EZ_ASSERT_DEV(cmd.m_LocalPoseOutput != ezInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    //EZ_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    EZ_ASSERT_DEV(cmd.m_LocalPoseOutput != ezInvalidIndex, "Output pose not allocated.");
    EZ_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::SampleTrack || type == ezAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    EZ_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    EZ_ASSERT_DEV(cmd.m_ModelPoseOutput != ezInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::SampleTrack || type == ezAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    EZ_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::LocalToModelPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    EZ_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    
  }
}

const ezAnimPoseGeneratorCommand& ezAnimPoseGenerator::GetCommand(ezAnimPoseGeneratorCommandID id) const
{
  return const_cast<ezAnimPoseGenerator*>(this)->GetCommand(id);
}

ezAnimPoseGeneratorCommand& ezAnimPoseGenerator::GetCommand(ezAnimPoseGeneratorCommandID id)
{
  EZ_ASSERT_DEV(id != ezInvalidIndex, "Invalid command ID");

  switch (GetCommandType(id))
  {
    case ezAnimPoseGeneratorCommandType::SampleTrack:
      return m_CommandsSampleTrack[GetCommandIndex(id)];

    case ezAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case ezAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case ezAnimPoseGeneratorCommandType::ModelPoseToOutput:
      return m_CommandsModelPoseToOutput[GetCommandIndex(id)];

    case ezAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  EZ_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

ezArrayPtr<ezMat4> ezAnimPoseGenerator::GeneratePose(const ezGameObject* pSendAnimationEventsTo /*= nullptr*/)
{
  Validate();

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    Execute(cmd, pSendAnimationEventsTo);
  }

  auto pPose = m_OutputPose;

  // TODO: clear temp data

  return pPose;
}

void ezAnimPoseGenerator::Execute(ezAnimPoseGeneratorCommand& cmd, const ezGameObject* pSendAnimationEventsTo)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id), pSendAnimationEventsTo);
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case ezAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandSampleTrack&>(cmd), pSendAnimationEventsTo);
      break;

    case ezAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case ezAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandLocalToModelPose&>(cmd));
      break;

    case ezAnimPoseGeneratorCommandType::ModelPoseToOutput:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandModelPoseToOutput&>(cmd));
      break;

    case ezAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandSampleEventTrack&>(cmd), pSendAnimationEventsTo);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd, const ezGameObject* pSendAnimationEventsTo)
{
  ezResourceLock<ezAnimationClipResource> pResource(cmd.m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  auto& pSampler = m_SamplingCaches[cmd.m_uiUniqueID];

  if (pSampler == nullptr)
  {
    pSampler = EZ_DEFAULT_NEW(ozz::animation::SamplingCache);
  }

  if (pSampler->max_tracks() != ozzAnim.num_tracks())
  {
    pSampler->Resize(ozzAnim.num_tracks());
  }

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.cache = pSampler;
  job.ratio = cmd.m_fNormalizedSamplePos;
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());

  if (!job.Validate())
    return;

  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  ezHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;

  for (ezUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    if (cmdIn.GetType() == ezAnimPoseGeneratorCommandType::SampleEventTrack)
      continue;

    ozz::animation::BlendingJob::Layer& layer = bl.ExpandAndGetRef();
    layer.weight = cmd.m_InputWeights[i];

    if (cmd.m_InputBoneWeights.GetCount() > i && !cmd.m_InputBoneWeights[i].IsEmpty())
    {
      layer.joint_weights = ozz::span(cmd.m_InputBoneWeights[i].GetPtr(), cmd.m_InputBoneWeights[i].GetEndPtr());
    }

    switch (cmdIn.GetType())
    {
      case ezAnimPoseGeneratorCommandType::SampleTrack:
      {
        auto transform = AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
        layer.transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case ezAnimPoseGeneratorCommandType::CombinePoses:
      {
        auto transform = AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
        layer.transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold = 1.0f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.bind_pose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_bind_poses();
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandLocalToModelPose& cmd)
{
  ozz::animation::LocalToModelJob job;

  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case ezAnimPoseGeneratorCommandType::SampleTrack:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

    case ezAnimPoseGeneratorCommandType::CombinePoses:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (cmd.m_pSendLocalPoseMsgTo)
  {
    ezMsgAnimationPosePreparing msg;
    msg.m_pSkeleton = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = ezMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (ezUInt32)job.input.size());

    cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  auto transforms = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);

  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(transforms.GetPtr()), transforms.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandModelPoseToOutput& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case ezAnimPoseGeneratorCommandType::LocalToModelPose:
      m_OutputPose = AcquireModelPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn).m_ModelPoseOutput);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandSampleEventTrack& cmd, const ezGameObject* pSendAnimationEventsTo)
{
  ezResourceLock<ezAnimationClipResource> pResource(cmd.m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void ezAnimPoseGenerator::SampleEventTrack(const ezAnimationClipResource* pResource, ezAnimPoseEventTrackSampleMode mode, const ezGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == ezAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const ezTime duration = pResource->GetDescriptor().GetDuration();

  const ezTime tPrev = fPrevPos * duration;
  const ezTime tNow = fCurPos * duration;
  const ezTime tStart = ezTime::Zero();
  const ezTime tEnd = duration + ezTime::Seconds(1.0); // sampling position is EXCLUSIVE

  ezHybridArray<ezHashedString, 16> events;

  switch (mode)
  {
    case ezAnimPoseEventTrackSampleMode::OnlyBetween:
      et.Sample(tPrev, tNow, events);
      break;

    case ezAnimPoseEventTrackSampleMode::LoopAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tStart, tNow, events);
      break;

    case ezAnimPoseEventTrackSampleMode::LoopAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

    case ezAnimPoseEventTrackSampleMode::BounceAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tEnd, tNow, events);
      break;

    case ezAnimPoseEventTrackSampleMode::BounceAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  ezMsgGenericEvent msg;

  for (const auto& hs : events)
  {
    msg.m_sMessage = hs;

    pSendAnimationEventsTo->SendEventMessage(msg, nullptr);
  }
}

ezArrayPtr<ozz::math::SoaTransform> ezAnimPoseGenerator::AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id].IsEmpty())
  {
    using T = ozz::math::SoaTransform;
    const ezUInt32 num = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints();
    m_UsedLocalTransforms[id] = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), T, num);
  }

  return m_UsedLocalTransforms[id];
}

ezArrayPtr<ezMat4> ezAnimPoseGenerator::AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  m_UsedModelTransforms[id].SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return m_UsedModelTransforms[id];
}
