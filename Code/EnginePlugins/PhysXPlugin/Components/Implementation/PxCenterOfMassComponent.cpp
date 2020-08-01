#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxCenterOfMassComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Shapes"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxCenterOfMassComponent::ezPxCenterOfMassComponent() = default;
ezPxCenterOfMassComponent::~ezPxCenterOfMassComponent() = default;

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCenterOfMassComponent);
