#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/FootIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFootIKComponent, 1, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("JointStart", m_sJointStart),
    EZ_MEMBER_PROPERTY("JointMiddle", m_sJointMiddle),
    EZ_MEMBER_PROPERTY("JointEnd", m_sJointEnd),
    EZ_ENUM_MEMBER_PROPERTY("MidAxis", ezBasisAxis, m_MidAxis),
    EZ_MEMBER_PROPERTY("PoleTarget", m_vPoleTarget),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Soften", m_fSoften)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("TwistAngle", m_TwistAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(-180), ezAngle::MakeFromDegree(180))),
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

ezFootIKComponent::ezFootIKComponent() = default;
ezFootIKComponent::~ezFootIKComponent() = default;

void ezFootIKComponent::SerializeComponent(ezWorldWriter& inout_stream) const
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

void ezFootIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
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

void ezFootIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  const ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const ezUInt16 uiEndJoint = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton.FindJointByName(m_sJointEnd);
  if (uiEndJoint == ezInvalidJointIndex)
    return;

  ezArrayPtr<ezMat4> pose = msg.m_pGenerator->GetCurrentPose();
  if (pose.IsEmpty())
    return;

  const ezVec3 vFootPos = msg.m_pOwner->GetGlobalTransform() * msg.m_SkeletonRootTransform * pose[uiEndJoint].GetTranslationVector();

  const float fThickness = 0.08f;

  ezPhysicsCastResult res;
  if (!pPhysicsInterface->Raycast(res, vFootPos + ezVec3(0, 0, 0.5f), ezVec3(0, 0, -1.0f), 0.5f + fThickness, ezPhysicsQueryParameters(0, ezPhysicsShapeType::Static)))
  {
    return;
  }

  ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(res.m_vPosition, 0.05f), ezColor::LawnGreen);
  res.m_vPosition.z += fThickness;

  const ezVec3 polePos = GetOwner()->GetGlobalTransform() * m_vPoleTarget;

  const ezTransform ownerTransform = ezTransform::MakeGlobalTransform(msg.m_pOwner->GetGlobalTransform(), msg.m_SkeletonRootTransform);
  const ezTransform localTarget = ezTransform::MakeLocalTransform(ownerTransform, res.m_vPosition);
  const ezTransform poleTarget = ezTransform::MakeLocalTransform(ownerTransform, ezTransform(polePos));

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

  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_sJointName = m_sJointEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVector = poleTarget.m_vPosition;
    cmdIk.m_fWeight = m_fWeight;
    cmdIk.m_vUpVector.Set(1, 0, 0);
    cmdIk.m_vForwardVector.Set(0, 1, 0);

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}
