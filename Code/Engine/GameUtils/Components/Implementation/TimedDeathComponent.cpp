#include <GameUtils/PCH.h>
#include <GameUtils/Components/TimedDeathComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezTimedDeathComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTimedDeathComponent::ezTimedDeathComponent()
{

}


void ezTimedDeathComponent::Update()
{

}

void ezTimedDeathComponent::SerializeComponent(ezWorldWriter& stream) const
{

}

void ezTimedDeathComponent::DeserializeComponent(ezWorldReader& stream)
{

}
