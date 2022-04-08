#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Components/JoltCenterOfMassComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltCenterOfMassComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Misc"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltCenterOfMassComponent::ezJoltCenterOfMassComponent() = default;
ezJoltCenterOfMassComponent::~ezJoltCenterOfMassComponent() = default;
