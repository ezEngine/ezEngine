#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/MarkerComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMarkerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Marker", GetMarkerType, SetMarkerType)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(0.1)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezSphereVisualizerAttribute("Radius", ezColor::LightSkyBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMarkerComponent::ezMarkerComponent() = default;
ezMarkerComponent::~ezMarkerComponent() = default;

void ezMarkerComponent::SetMarkerType(const char* szType)
{
  m_sMarkerType.Assign(szType);

  UpdateMarker();
}

const char* ezMarkerComponent::GetMarkerType() const
{
  return m_sMarkerType;
}

void ezMarkerComponent::SetRadius(float radius)
{
  m_fRadius = radius;

  UpdateMarker();
}

float ezMarkerComponent::GetRadius() const
{
  return m_fRadius;
}

void ezMarkerComponent::OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0), m_fRadius), m_SpatialCategory);
}

void ezMarkerComponent::UpdateMarker()
{
  if (!m_sMarkerType.IsEmpty())
  {
    m_SpatialCategory = ezSpatialData::RegisterCategory(m_sMarkerType.GetString());
  }
  else
  {
    m_SpatialCategory = ezInvalidSpatialDataCategory;
  }

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezMarkerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sMarkerType;
  s << m_fRadius;
}

void ezMarkerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_sMarkerType;
  s >> m_fRadius;
}

void ezMarkerComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMarker();
}

void ezMarkerComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_MarkerComponent);
