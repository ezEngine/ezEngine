#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/StaticMeshComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezStaticMeshComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezStaticMeshComponent::ezStaticMeshComponent()
{
}


void ezStaticMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezStaticMeshComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);



}

ezResult ezStaticMeshComponent::Initialize()
{
	int i = 0;
	return EZ_SUCCESS;
}

ezResult ezStaticMeshComponent::Deinitialize()
{
	int i = 0;
	return EZ_SUCCESS;
}

