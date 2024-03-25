#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AimIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAimIKComponent, 1, ezComponentMode::Dynamic);
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_MEMBER_PROPERTY("OverrideScale", m_bOverrideScale)->AddAttributes(new ezDefaultValueAttribute(false)),
  //}
  //EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseGeneration, OnMsgAnimationPoseGeneration)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAimIKComponent::ezAimIKComponent() = default;
ezAimIKComponent::~ezAimIKComponent() = default;

void ezAimIKComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void ezAimIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}

void ezAimIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  const ezTransform ownerTransform = ezTransform::MakeGlobalTransform(msg.m_pOwner->GetGlobalTransform(), msg.m_SkeletonRootTransform);
  const ezTransform localTarget = ezTransform::MakeLocalTransform(ownerTransform, GetOwner()->GetGlobalTransform());

  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_sJointName = "UpperArm.R";
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_fWeight = 0.8f;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }

  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_sJointName = "LowerArm.R";
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}
