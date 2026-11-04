#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/Components/AsteroidsGameComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ProjectileComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(70.0f)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Asteroids"),
    new ezSphereVisualizerAttribute("Radius", ezColor::Orange),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ProjectileComponent::ProjectileComponent() = default;
ProjectileComponent::~ProjectileComponent() = default;

void ProjectileComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fSpeed;
  s << m_fRadius;
}

void ProjectileComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fSpeed;
  s >> m_fRadius;
}

void ProjectileComponent::Update()
{
  AsteroidsGameComponent::MoveAndWrap(GetOwner(), m_vVelocity, m_fRadius);
}
