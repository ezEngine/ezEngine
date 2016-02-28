#include <GameUtils/PCH.h>
#include <GameUtils/Components/CameraComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezCameraComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Rendering"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCameraComponent* ezCameraComponent::s_pCurrent = nullptr;

ezCameraComponent::ezCameraComponent()
{
}

ezCameraComponent::~ezCameraComponent()
{
  s_pCurrent = nullptr;
}

void ezCameraComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

}

void ezCameraComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();


}

ezComponent::Initialization ezCameraComponent::Initialize()
{
  s_pCurrent = this;

  return ezComponent::Initialization::Done;
}

