#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxCenterOfMassComponent, 1, ezComponentMode::Static)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezPxCenterOfMassComponent::ezPxCenterOfMassComponent() = default;
ezPxCenterOfMassComponent::~ezPxCenterOfMassComponent() = default;

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCenterOfMassComponent);
