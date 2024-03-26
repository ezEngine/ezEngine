#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/TwoBoneIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTwoBoneIKComponent, 1, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("JointStart", m_sJointStart),
    EZ_MEMBER_PROPERTY("JointMiddle", m_sJointMiddle),
    EZ_MEMBER_PROPERTY("JointEnd", m_sJointEnd),
    EZ_ENUM_MEMBER_PROPERTY("MidAxis", ezBasisAxis, m_MidAxis),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Soften", m_fSoften)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("TwistAngle", m_TwistAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(-180), ezAngle::MakeFromDegree(180))),
    EZ_MEMBER_PROPERTY("PoleTarget", m_vPoleTarget),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
      new ezTransformManipulatorAttribute("PoleTarget"),
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

ezTwoBoneIKComponent::ezTwoBoneIKComponent() = default;
ezTwoBoneIKComponent::~ezTwoBoneIKComponent() = default;

void ezTwoBoneIKComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_fSoften;
  s << m_sJointStart;
  s << m_sJointMiddle;
  s << m_sJointEnd;
  s << m_MidAxis;
  s << m_TwistAngle;
}

void ezTwoBoneIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_fSoften;
  s >> m_sJointStart;
  s >> m_sJointMiddle;
  s >> m_sJointEnd;
  s >> m_MidAxis;
  s >> m_TwistAngle;
}

void ezTwoBoneIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  const ezVec3 polePos = GetOwner()->GetGlobalTransform() * m_vPoleTarget;

  const ezTransform ownerTransform = ezTransform::MakeGlobalTransform(msg.m_pOwner->GetGlobalTransform(), msg.m_SkeletonRootTransform);
  const ezTransform localTarget = ezTransform::MakeLocalTransform(ownerTransform, GetOwner()->GetGlobalTransform());
  const ezTransform poleTarget = ezTransform::MakeLocalTransform(ownerTransform, ezTransform(polePos));

  ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(GetOwner()->GetGlobalTransform() * m_vPoleTarget, 0.1f), ezColor::Pink);


  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandTwoBoneIK();
    cmdIk.m_sJointNameStart = m_sJointStart;
    cmdIk.m_sJointNameMiddle = m_sJointMiddle;
    cmdIk.m_sJointNameEnd = m_sJointEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVector = poleTarget.m_vPosition;
    cmdIk.m_vMidAxis = ezBasisAxis::GetBasisVector(m_MidAxis);
    cmdIk.m_fWeight = m_fWeight;
    cmdIk.m_fSoften = m_fSoften;
    cmdIk.m_TwistAngle = m_TwistAngle;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}
