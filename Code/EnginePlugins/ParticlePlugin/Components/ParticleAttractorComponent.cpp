#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/SpatialData.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleAttractorComponent.h>

static ezSpatialData::Category s_AttractorSpatialCategory = ezSpatialData::RegisterCategory("ParticleAttractor", ezSpatialData::Flags::None);

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleAttractorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.01f, {})),
    EZ_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("MinDistance", m_fMinDistance)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.001f, {})),
    EZ_MEMBER_PROPERTY("KillDistance", m_fKillDistance)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, {})),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereManipulatorAttribute("KillDistance"),
    new ezSphereVisualizerAttribute("Radius", ezColor::CornflowerBlue),
    new ezSphereVisualizerAttribute("KillDistance", ezColor::OrangeRed),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezParticleAttractorComponent::ezParticleAttractorComponent() = default;
ezParticleAttractorComponent::~ezParticleAttractorComponent() = default;

// static
ezSpatialData::Category ezParticleAttractorComponent::GetSpatialCategory()
{
  return s_AttractorSpatialCategory;
}

void ezParticleAttractorComponent::OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fRadius), s_AttractorSpatialCategory);
}

void ezParticleAttractorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fStrength;
  s << m_fMinDistance;
  s << m_fKillDistance;
}

void ezParticleAttractorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fStrength;
  s >> m_fMinDistance;
  s >> m_fKillDistance;
}

void ezParticleAttractorComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->UpdateLocalBounds();
}

void ezParticleAttractorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
  GetOwner()->UpdateLocalBounds();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleAttractorComponent);
