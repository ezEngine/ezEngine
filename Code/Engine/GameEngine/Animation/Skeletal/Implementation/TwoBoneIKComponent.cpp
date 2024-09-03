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
    EZ_ENUM_MEMBER_PROPERTY("MidAxis", ezBasisAxis, m_MidAxis)->AddAttributes(new ezDefaultValueAttribute(ezBasisAxis::PositiveZ)),
    EZ_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    //EZ_MEMBER_PROPERTY("Soften", m_fSoften)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    //EZ_MEMBER_PROPERTY("TwistAngle", m_TwistAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(-180), ezAngle::MakeFromDegree(180))),
  }
  EZ_END_PROPERTIES;

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

ezTwoBoneIKComponent::ezTwoBoneIKComponent() = default;
ezTwoBoneIKComponent::~ezTwoBoneIKComponent() = default;

void ezTwoBoneIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void ezTwoBoneIKComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_sJointStart;
  s << m_sJointMiddle;
  s << m_sJointEnd;
  s << m_MidAxis;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);
  // s << m_fSoften;
  // s << m_TwistAngle;
}

void ezTwoBoneIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_sJointStart;
  s >> m_sJointMiddle;
  s >> m_sJointEnd;
  s >> m_MidAxis;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();
  // s >> m_fSoften;
  // s >> m_TwistAngle;
}

void ezTwoBoneIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  const ezTransform targetTrans = msg.m_pGenerator->GetTargetObject()->GetGlobalTransform();
  const ezTransform ownerTransform = ezTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const ezTransform localTarget = ezTransform::MakeLocalTransform(ownerTransform, GetOwner()->GetGlobalTransform());

  ezVec3 vPoleVectorPos;

  const ezGameObject* pPoleVector;
  if (!m_hPoleVector.IsInvalidated() && GetWorld()->TryGetObject(m_hPoleVector, pPoleVector))
  {
    vPoleVectorPos = ezTransform::MakeLocalTransform(ownerTransform, ezTransform(pPoleVector->GetGlobalPosition())).m_vPosition;
  }
  else
  {
    // hard-coded "up vector" as pole target
    vPoleVectorPos = ezTransform::MakeLocalTransform(ownerTransform, ezTransform(targetTrans * ezVec3(0, 0, 10))).m_vPosition;
  }

  if (m_uiJointIdxStart == 0 && m_uiJointIdxMiddle == 0)
  {
    auto& skel = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton;
    m_uiJointIdxStart = skel.FindJointByName(m_sJointStart);
    m_uiJointIdxMiddle = skel.FindJointByName(m_sJointMiddle);
    m_uiJointIdxEnd = skel.FindJointByName(m_sJointEnd);
  }

  if (m_uiJointIdxStart != ezInvalidJointIndex && m_uiJointIdxMiddle != ezInvalidJointIndex && m_uiJointIdxEnd != ezInvalidJointIndex)
  {
    auto& cmdIk = msg.m_pGenerator->AllocCommandTwoBoneIK();
    cmdIk.m_uiJointIdxStart = m_uiJointIdxStart;
    cmdIk.m_uiJointIdxMiddle = m_uiJointIdxMiddle;
    cmdIk.m_uiJointIdxEnd = m_uiJointIdxEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVector = vPoleVectorPos;
    cmdIk.m_vMidAxis = ezBasisAxis::GetBasisVector(m_MidAxis);
    cmdIk.m_fWeight = m_fWeight;
    cmdIk.m_fSoften = 1.0f;                   // m_fSoften;
    cmdIk.m_TwistAngle = ezAngle::MakeZero(); // m_TwistAngle;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_TwoBoneIKComponent);

