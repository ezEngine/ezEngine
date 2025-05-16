#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AimIKComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezIkJointEntry, ezNoBase, 1, ezRTTIDefaultAllocator<ezIkJointEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Joint", m_sJointName),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezAimIKComponent, 2, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("DebugVisScale", GetDebugVisScale, SetDebugVisScale)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_ENUM_MEMBER_PROPERTY("ForwardVector", ezBasisAxis, m_ForwardVector)->AddAttributes(new ezDefaultValueAttribute(ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpVector", ezBasisAxis, m_UpVector)->AddAttributes(new ezDefaultValueAttribute(ezBasisAxis::PositiveZ)),
    EZ_ACCESSOR_PROPERTY("PoleVector", DummyGetter, SetPoleVectorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("InversePoleVector", m_bInversePoleVector),
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ARRAY_MEMBER_PROPERTY("Joints", m_Joints),
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

ezResult ezIkJointEntry::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sJointName;
  inout_stream << m_fWeight;
  return EZ_SUCCESS;
}

ezResult ezIkJointEntry::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sJointName;
  inout_stream >> m_fWeight;
  m_uiJointIdx = 0;
  return EZ_SUCCESS;
}

ezAimIKComponent::ezAimIKComponent() = default;
ezAimIKComponent::~ezAimIKComponent() = default;

void ezAimIKComponent::SetPoleVectorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPoleVector = resolver(szReference, GetHandle(), "PoleVector");
}

void ezAimIKComponent::SetDebugVisScale(float fScale)
{
  // allow scales from 0.05f to 10.0f
  // map them to range 0 to 200
  m_uiDebugVisScale = static_cast<ezUInt8>(ezMath::Clamp(ezMath::RoundToInt(fScale * 20.0f), 0, 200));
}

float ezAimIKComponent::GetDebugVisScale() const
{
  return m_uiDebugVisScale / 20.0f;
}

void ezAimIKComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fWeight;
  s << m_ForwardVector;
  s << m_UpVector;
  inout_stream.WriteGameObjectHandle(m_hPoleVector);
  s.WriteArray(m_Joints).AssertSuccess();

  s << m_bInversePoleVector;
}

void ezAimIKComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fWeight;
  s >> m_ForwardVector;
  s >> m_UpVector;
  m_hPoleVector = inout_stream.ReadGameObjectHandle();
  s.ReadArray(m_Joints).AssertSuccess();

  if (uiVersion >= 2)
  {
    s >> m_bInversePoleVector;
  }
}

void ezAimIKComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& msg) const
{
  if ((m_fWeight <= 0.0f && m_uiDebugVisScale == 0) || m_Joints.IsEmpty())
    return;

  const ezTransform targetTrans = msg.m_pGenerator->GetTargetObject()->GetGlobalTransform();
  const ezTransform selfTrans = GetOwner()->GetGlobalTransform();
  const ezTransform ownerTransform = ezTransform::MakeGlobalTransform(targetTrans, msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_RootTransform);
  const ezTransform localTarget = ezTransform::MakeLocalTransform(ownerTransform, selfTrans);

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

  for (ezUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].m_fWeight <= 0.0f)
      continue;

    if (m_Joints[i].m_uiJointIdx == 0)
    {
      m_Joints[i].m_uiJointIdx = msg.m_pGenerator->GetSkeleton()->GetDescriptor().m_Skeleton.FindJointByName(m_Joints[i].m_sJointName);
    }

    if (m_Joints[i].m_uiJointIdx == ezInvalidJointIndex)
      continue;

    auto& cmdIk = msg.m_pGenerator->AllocCommandAimIK();
    cmdIk.m_fDebugVisScale = GetDebugVisScale();
    cmdIk.m_uiJointIdx = m_Joints[i].m_uiJointIdx;
    cmdIk.m_Inputs.PushBack(msg.m_pGenerator->GetFinalCommand());
    cmdIk.m_vTargetPosition = localTarget.m_vPosition;
    cmdIk.m_fWeight = m_fWeight * m_Joints[i].m_fWeight;
    cmdIk.m_vForwardVector = ezBasisAxis::GetBasisVector(m_ForwardVector);
    cmdIk.m_vUpVector = ezBasisAxis::GetBasisVector(m_UpVector);
    cmdIk.m_vPoleVectorPosition = vPoleVectorPos;
    cmdIk.m_bInversePoleVector = m_bInversePoleVector;

    // in theory one could limit which joints get their model poses updated,
    // but in practice this doesn't work unless we know that they are definitely just in one straight line (not the case for spines)
    // if (i + 1 < m_Joints.GetCount())
    //{
    //  cmdIk.m_uiRecalcModelPoseToJointIdx = m_Joints[i + 1].m_uiJointIdx;
    //}

    msg.m_pGenerator->SetFinalCommand(cmdIk.GetCommandID());
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AimIKComponent);
