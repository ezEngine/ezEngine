#include <PCH.h>
#include <GameEngine/Components/ShapeIconComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezShapeIconComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute() // don't show in UI
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezShapeIconComponent::ezShapeIconComponent()
{
}

ezShapeIconComponent::~ezShapeIconComponent()
{
}

void ezShapeIconComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();
}

void ezShapeIconComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  //ezStreamReader& s = stream.GetStream();
}
