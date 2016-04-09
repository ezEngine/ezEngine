#include <GameUtils/PCH.h>
#include <GameUtils/Components/ProjectileComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::OrangeRed)
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezProjectileComponent::ezProjectileComponent()
{
}


void ezProjectileComponent::Update()
{

}

void ezProjectileComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

}

void ezProjectileComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

}

