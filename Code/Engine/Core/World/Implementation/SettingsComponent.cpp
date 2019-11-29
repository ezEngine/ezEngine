#include <CorePCH.h>

#include <Core/World/SettingsComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSettingsComponent, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Settings"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSettingsComponent::ezSettingsComponent()
{
  SetModified();
}

ezSettingsComponent::~ezSettingsComponent() = default;


EZ_STATICLINK_FILE(Core, Core_World_Implementation_SettingsComponent);
