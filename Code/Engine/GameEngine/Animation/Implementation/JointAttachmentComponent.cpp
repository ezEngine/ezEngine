#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/JointAttachmentComponent.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJointAttachmentComponent, 2, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJointAttachmentComponent::ezJointAttachmentComponent() = default;
ezJointAttachmentComponent::~ezJointAttachmentComponent() = default;

void ezJointAttachmentComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sJointToAttachTo;
}

void ezJointAttachmentComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion >= 2)
  {
    s >> m_sJointToAttachTo;
  }

  m_uiJointIndex = ezInvalidJointIndex;
}

void ezJointAttachmentComponent::SetJointName(const char* szName)
{
  m_sJointToAttachTo.Assign(szName);
  m_uiJointIndex = ezInvalidJointIndex;
}

const char* ezJointAttachmentComponent::GetJointName() const
{
  return m_sJointToAttachTo.GetData();
}

void ezJointAttachmentComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  if (m_uiJointIndex == ezInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToAttachTo);
  }

  if (m_uiJointIndex == ezInvalidJointIndex)
    return;

  const ezMat4 m = msg.m_pPose->GetTransform(m_uiJointIndex);
  ezTransform t;
  t.SetFromMat4(m);

  ezGameObject* pOwner = GetOwner();
  pOwner->SetLocalPosition(t.m_vPosition);
  pOwner->SetLocalRotation(t.m_qRotation);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_JointAttachmentComponent);

