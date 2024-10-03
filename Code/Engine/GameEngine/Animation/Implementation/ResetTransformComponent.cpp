#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ResetTransformComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezResetTransformComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ResetPositionX", m_bResetLocalPositionX)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ResetPositionY", m_bResetLocalPositionY)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ResetPositionZ", m_bResetLocalPositionZ)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    EZ_MEMBER_PROPERTY("ResetRotation", m_bResetLocalRotation)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation)->AddAttributes(new ezDefaultValueAttribute(ezQuat::MakeIdentity())),
    EZ_MEMBER_PROPERTY("ResetScaling", m_bResetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LocalScaling", m_vLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1))),
    EZ_MEMBER_PROPERTY("LocalUniformScaling", m_fLocalUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResetTransformComponent::ezResetTransformComponent() = default;
ezResetTransformComponent::~ezResetTransformComponent() = default;

void ezResetTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezVec3 vLocalPos = GetOwner()->GetLocalPosition();

  if (m_bResetLocalPositionX)
    vLocalPos.x = m_vLocalPosition.x;
  if (m_bResetLocalPositionY)
    vLocalPos.y = m_vLocalPosition.y;
  if (m_bResetLocalPositionZ)
    vLocalPos.z = m_vLocalPosition.z;

  GetOwner()->SetLocalPosition(vLocalPos);

  if (m_bResetLocalRotation)
  {
    GetOwner()->SetLocalRotation(m_qLocalRotation);
  }

  if (m_bResetLocalScaling)
  {
    GetOwner()->SetLocalScaling(m_vLocalScaling);
    GetOwner()->SetLocalUniformScaling(m_fLocalUniformScaling);
  }

  // update the global transform right away
  GetOwner()->UpdateGlobalTransform();
}

void ezResetTransformComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_vLocalPosition;
  s << m_qLocalRotation;
  s << m_vLocalScaling;
  s << m_bResetLocalPositionX;
  s << m_bResetLocalPositionY;
  s << m_bResetLocalPositionZ;
  s << m_bResetLocalRotation;
  s << m_bResetLocalScaling;
  s << m_fLocalUniformScaling;
}

void ezResetTransformComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_vLocalPosition;
  s >> m_qLocalRotation;
  s >> m_vLocalScaling;
  s >> m_bResetLocalPositionX;
  s >> m_bResetLocalPositionY;
  s >> m_bResetLocalPositionZ;
  s >> m_bResetLocalRotation;
  s >> m_bResetLocalScaling;
  s >> m_fLocalUniformScaling;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ResetTransformComponent);
