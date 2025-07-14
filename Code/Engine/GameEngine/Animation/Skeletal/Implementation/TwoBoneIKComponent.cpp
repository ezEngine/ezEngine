#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/TwoBoneIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTwoBoneIKComponent, 3, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("DebugVisScale", GetDebugVisScale, SetDebugVisScale)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("JointStart", m_sJointStart),
    EZ_MEMBER_PROPERTY("JointMiddle", m_sJointMiddle),
    EZ_MEMBER_PROPERTY("JointEnd", m_sJointEnd),
    EZ_ENUM_MEMBER_PROPERTY("MidAxis", ezBasisAxis, m_MidAxis)->AddAttributes(new ezDefaultValueAttribute(ezBasisAxis::PositiveZ)),
    EZ_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Order", m_uiOrder),
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

void ezTwoBoneIKComponent::SetDebugVisScale(float fScale)
{
  // allow scales from 0.05f to 10.0f
  // map them to range 0 to 200
  m_uiDebugVisScale = static_cast<ezUInt8>(ezMath::Clamp(ezMath::RoundToInt(fScale * 20.0f), 0, 200));
}

float ezTwoBoneIKComponent::GetDebugVisScale() const
{
  return m_uiDebugVisScale / 20.0f;
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

  // version 2
  s << m_uiDebugVisScale;

  // s << m_fSoften;
  // s << m_TwistAngle;

  // version 3
  s << m_uiOrder;
}

void ezTwoBoneIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_sJointStart;
  s >> m_sJointMiddle;
  s >> m_sJointEnd;
  s >> m_MidAxis;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_uiDebugVisScale;
  }

  // s >> m_fSoften;
  // s >> m_TwistAngle;

  if (uiVersion >= 3)
  {
    s >> m_uiOrder;
  }
}

void ezTwoBoneIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  if (m_fWeight <= 0.0f && m_uiDebugVisScale == 0)
    return;

  // if we are already past this, just return
  if (m_uiOrder < msg.m_uiOrderNow)
    return;

  // if we haven't reached this yet, put it in the queue
  if (m_uiOrder > msg.m_uiOrderNow)
  {
    msg.m_uiOrderNext = ezMath::Min(msg.m_uiOrderNext, m_uiOrder);
    return;
  }

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
    cmdIk.m_fDebugVisScale = GetDebugVisScale();
    cmdIk.m_uiJointIdxStart = m_uiJointIdxStart;
    cmdIk.m_uiJointIdxMiddle = m_uiJointIdxMiddle;
    cmdIk.m_uiJointIdxEnd = m_uiJointIdxEnd;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_vPoleVectorPosition = vPoleVectorPos;
    cmdIk.m_vMidAxis = ezBasisAxis::GetBasisVector(m_MidAxis);
    cmdIk.m_fWeight = m_fWeight;
    cmdIk.m_fSoften = 1.0f;                   // m_fSoften;
    cmdIk.m_TwistAngle = ezAngle::MakeZero(); // m_TwistAngle;

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_TwoBoneIKComponent);
