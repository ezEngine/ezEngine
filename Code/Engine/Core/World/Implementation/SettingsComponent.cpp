#include <Core/PCH.h>
#include <Core/World/SettingsComponent.h>
//#include <Core/WorldSerializer/WorldWriter.h>
//#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSettingsComponent, 1, ezRTTINoAllocator)
{
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Settings"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSettingsComponent::ezSettingsComponent()
{
  SetModified();
}

void ezSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

}

