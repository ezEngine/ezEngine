#include <PhysXPluginPCH.h>

#include <PhysXPlugin/Components/PxComponent.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxComponent, 1)
{
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezPxComponent::ezPxComponent() = default;
ezPxComponent::~ezPxComponent() = default;


EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxComponent);
