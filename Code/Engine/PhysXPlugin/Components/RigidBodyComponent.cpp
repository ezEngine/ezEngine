#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/RigidBodyComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezRigidBodyComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRigidBodyComponent::ezRigidBodyComponent()
{
}


void ezRigidBodyComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezRigidBodyComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);



}

ezResult ezRigidBodyComponent::Initialize()
{
	int i = 0;
	return EZ_SUCCESS;
}

ezResult ezRigidBodyComponent::Deinitialize()
{
	int i = 0;
	return EZ_SUCCESS;
}

