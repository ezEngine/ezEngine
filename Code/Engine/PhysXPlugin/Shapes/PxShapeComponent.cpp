#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxShapeComponent::ezPxShapeComponent()
{
}


void ezPxShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxShapeComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);



}
