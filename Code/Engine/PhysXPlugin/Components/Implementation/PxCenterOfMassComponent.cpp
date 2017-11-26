#include <PCH.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxCenterOfMassComponent, 1, ezComponentMode::Static);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxCenterOfMassComponent::ezPxCenterOfMassComponent()
{
}


void ezPxCenterOfMassComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxCenterOfMassComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());




}





EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCenterOfMassComponent);

