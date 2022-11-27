#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointAttachmentComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJointAttachmentComponent, 1, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    EZ_MEMBER_PROPERTY("PositionOffset", m_vLocalPositionOffset),
    EZ_MEMBER_PROPERTY("RotationOffset", m_vLocalRotationOffset),
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

void ezJointAttachmentComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToAttachTo;
  s << m_vLocalPositionOffset;
  s << m_vLocalRotationOffset;
}

void ezJointAttachmentComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToAttachTo;
  s >> m_vLocalPositionOffset;
  s >> m_vLocalRotationOffset;

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

  ezMat4 bone;
  ezQuat boneRot;

  msg.ComputeFullBoneTransform(m_uiJointIndex, bone, boneRot);

  ezGameObject* pOwner = GetOwner();
  pOwner->SetLocalPosition(bone.GetTranslationVector() + bone.TransformDirection(m_vLocalPositionOffset));
  pOwner->SetLocalRotation(boneRot * m_vLocalRotationOffset);
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointAttachmentComponent);
