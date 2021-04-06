#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <AnimationSystem/Declarations.h>
#include <AnimationSystem/SkeletonResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

ezAnimPoseGeneratorCommand::ezAnimPoseGeneratorCommand() = default;
ezAnimPoseGeneratorCommand::~ezAnimPoseGeneratorCommand() = default;

void ezAnimPoseGeneratorCommand::ConfigureCmd(ezAnimPoseGeneratorCommandType type, ezAnimPoseGeneratorCommandID cmdId)
{
  m_Type = type;
  m_CommandID = cmdId;
}

ezAnimPoseGeneratorCommandSampleTrack::ezAnimPoseGeneratorCommandSampleTrack() = default;
ezAnimPoseGeneratorCommandSampleTrack::~ezAnimPoseGeneratorCommandSampleTrack() = default;

void ezAnimPoseGeneratorCommandSampleTrack::Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorLocalPoseID poseID)
{
  ConfigureCmd(ezAnimPoseGeneratorCommandType::SampleTrack, cmdId);
  m_LocalPoseOutput = poseID;
}

void ezAnimPoseGeneratorCommandSampleTrack::Validate(const ezAnimPoseGenerator& queue) const
{
  EZ_ASSERT_DEV(m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  EZ_ASSERT_DEV(m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
  EZ_ASSERT_DEV(m_LocalPoseOutput != ezInvalidIndex, "Output pose not allocated.");
}

ezAnimPoseGeneratorCommandCombinePoses::ezAnimPoseGeneratorCommandCombinePoses() = default;
ezAnimPoseGeneratorCommandCombinePoses::~ezAnimPoseGeneratorCommandCombinePoses() = default;

void ezAnimPoseGeneratorCommandCombinePoses::Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorLocalPoseID poseID)
{
  ConfigureCmd(ezAnimPoseGeneratorCommandType::CombinePoses, cmdId);
  m_LocalPoseOutput = poseID;
}

void ezAnimPoseGeneratorCommandCombinePoses::Validate(const ezAnimPoseGenerator& queue) const
{
  EZ_ASSERT_DEV(m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
  EZ_ASSERT_DEV(m_LocalPoseOutput != ezInvalidIndex, "Output pose not allocated.");
  EZ_ASSERT_DEV(m_Inputs.GetCount() == m_InputWeights.GetCount(), "Number of inputs and weights must match.");

  for (auto id : m_Inputs)
  {
    auto type = queue.GetCommand(id).GetType();
    EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::SampleTrack || type == ezAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
  }
}

ezAnimPoseGeneratorCommandLocalToModelPose::ezAnimPoseGeneratorCommandLocalToModelPose() = default;
ezAnimPoseGeneratorCommandLocalToModelPose::~ezAnimPoseGeneratorCommandLocalToModelPose() = default;

void ezAnimPoseGeneratorCommandLocalToModelPose::Configure(ezAnimPoseGeneratorCommandID cmdId, ezAnimPoseGeneratorModelPoseID poseID)
{
  ConfigureCmd(ezAnimPoseGeneratorCommandType::LocalToModelPose, cmdId);
  m_ModelPoseOutput = poseID;
}

void ezAnimPoseGeneratorCommandLocalToModelPose::Validate(const ezAnimPoseGenerator& queue) const
{
  EZ_ASSERT_DEV(m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
  EZ_ASSERT_DEV(m_ModelPoseOutput != ezInvalidIndex, "Output pose not allocated.");

  for (auto id : m_Inputs)
  {
    auto type = queue.GetCommand(id).GetType();
    EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::SampleTrack || type == ezAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
  }
}

ezAnimPoseGeneratorCommandModelPoseToOutput::ezAnimPoseGeneratorCommandModelPoseToOutput() = default;
ezAnimPoseGeneratorCommandModelPoseToOutput::~ezAnimPoseGeneratorCommandModelPoseToOutput() = default;

void ezAnimPoseGeneratorCommandModelPoseToOutput::Configure(ezAnimPoseGeneratorCommandID cmdId)
{
  ConfigureCmd(ezAnimPoseGeneratorCommandType::ModelPoseToOutput, cmdId);
}

void ezAnimPoseGeneratorCommandModelPoseToOutput::Validate(const ezAnimPoseGenerator& queue) const
{
  EZ_ASSERT_DEV(m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

  for (auto id : m_Inputs)
  {
    auto type = queue.GetCommand(id).GetType();
    EZ_ASSERT_DEV(type == ezAnimPoseGeneratorCommandType::LocalToModelPose, "Unsupported input type");
  }
}

void ezAnimPoseGenerator::Reset(const ezSkeletonResource* pSkeleton)
{
  m_pSkeleton = pSkeleton;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;

  m_Commands.Clear();
  m_CommandsSampleTrack.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsModelPoseToOutput.Clear();

  m_LocalTransforms.Clear(); // TODO: share memory across queues and runs
  m_UsedLocalTransforms.Clear();

  m_ModelTransforms.Clear();
  m_UsedModelTransforms.Clear();

  m_pOutputPose = nullptr;
}

ezAnimPoseGeneratorCommandSampleTrack& ezAnimPoseGenerator::AllocCommandSampleTrack()
{
  auto& cmd = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.Configure(m_Commands.GetCount(), m_LocalPoseCounter++);
  m_Commands.PushBack(&cmd);

  return cmd;
}

ezAnimPoseGeneratorCommandCombinePoses& ezAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.Configure(m_Commands.GetCount(), m_LocalPoseCounter++);
  m_Commands.PushBack(&cmd);

  return cmd;
}

ezAnimPoseGeneratorCommandLocalToModelPose& ezAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.Configure(m_Commands.GetCount(), m_ModelPoseCounter++);
  m_Commands.PushBack(&cmd);

  return cmd;
}

ezAnimPoseGeneratorCommandModelPoseToOutput& ezAnimPoseGenerator::AllocCommandModelPoseToOutput()
{
  auto& cmd = m_CommandsModelPoseToOutput.ExpandAndGetRef();
  cmd.Configure(m_Commands.GetCount());
  m_Commands.PushBack(&cmd);
  return cmd;
}

void ezAnimPoseGenerator::Validate() const
{
  EZ_ASSERT_DEV(m_CommandsModelPoseToOutput.GetCount() <= 1, "Only one output node may exist");

  for (auto& cmd : m_CommandsSampleTrack)
    cmd.Validate(*this);

  for (auto& cmd : m_CommandsCombinePoses)
    cmd.Validate(*this);

  for (auto& cmd : m_CommandsLocalToModelPose)
    cmd.Validate(*this);

  for (auto& cmd : m_CommandsModelPoseToOutput)
    cmd.Validate(*this);
}

const ezAnimPoseGeneratorCommand& ezAnimPoseGenerator::GetCommand(ezAnimPoseGeneratorCommandID id) const
{
  return *m_Commands[id];
}

ezAnimPoseGeneratorCommand& ezAnimPoseGenerator::GetCommand(ezAnimPoseGeneratorCommandID id)
{
  return *m_Commands[id];
}

ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>* ezAnimPoseGenerator::GeneratePose()
{
  Validate();

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    Execute(cmd);
  }

  auto pPose = m_pOutputPose;

  // TODO: clear temp data

  return pPose;
}

void ezAnimPoseGenerator::Execute(ezAnimPoseGeneratorCommand& cmd)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id));
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case ezAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<ezAnimPoseGeneratorCommandSampleTrack&>(cmd));
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

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  cmd.m_bExecuted = true;
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandSampleTrack& cmd)
{
  ezResourceLock<ezAnimationClipResource> pResource(cmd.m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  auto pTransforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  // TODO: reuse cache
  ozz::animation::SamplingCache cache;
  cache.Resize(ozzAnim.num_tracks());

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.cache = &cache;
  job.ratio = cmd.m_SampleTime.AsFloatInSeconds() / pResource->GetDescriptor().GetDuration().AsFloatInSeconds();
  job.output = make_span(*pTransforms);
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void ezAnimPoseGenerator::ExecuteCmd(ezAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto pTransforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  ezHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;

  for (ezUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    auto& layer = bl.ExpandAndGetRef();
    layer.weight = cmd.m_InputWeights[i];

    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    switch (cmdIn.GetType())
    {
      case ezAnimPoseGeneratorCommandType::SampleTrack:
        layer.transform = make_span(*AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput));
        break;

      case ezAnimPoseGeneratorCommandType::CombinePoses:
        layer.transform = make_span(*AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput));
        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold = 0.1f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.bind_pose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_bind_poses();
  job.output = make_span(*pTransforms);
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
      job.input = make_span(*AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput));
      break;

    case ezAnimPoseGeneratorCommandType::CombinePoses:
      job.input = make_span(*AcquireLocalPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput));
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

  auto pTransforms = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);

  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(begin(*pTransforms)), reinterpret_cast<ozz::math::Float4x4*>(end(*pTransforms)));
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
      m_pOutputPose = AcquireModelPoseTransforms(static_cast<const ezAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn).m_ModelPoseOutput);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

ozz::vector<ozz::math::SoaTransform>* ezAnimPoseGenerator::AcquireLocalPoseTransforms(ezAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id] == nullptr)
  {
    m_UsedLocalTransforms[id] = &m_LocalTransforms.ExpandAndGetRef();
    m_UsedLocalTransforms[id]->resize(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
  }

  return m_UsedLocalTransforms[id];
}

ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper>* ezAnimPoseGenerator::AcquireModelPoseTransforms(ezAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  if (m_UsedModelTransforms[id] == nullptr)
  {
    m_UsedModelTransforms[id] = &m_ModelTransforms.ExpandAndGetRef();
    m_UsedModelTransforms[id]->SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());
  }

  return m_UsedModelTransforms[id];
}
