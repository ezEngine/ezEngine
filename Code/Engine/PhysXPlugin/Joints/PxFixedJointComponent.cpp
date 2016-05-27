#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Joints/PxFixedJointComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/Messages/CallDelayedStartMessage.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxFixedJointComponent, 1);
{

}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxFixedJointComponent::ezPxFixedJointComponent()
{
}


void ezPxFixedJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

}


void ezPxFixedJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

}

void ezPxFixedJointComponent::OnSimulationStarted()
{
  PxFixedJoint* pJoint = static_cast<PxFixedJoint*>(SetupJoint());

  if (pJoint == nullptr)
    return;

}

PxJoint* ezPxFixedJointComponent::CreateJointType(PxPhysics& api, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  return PxFixedJointCreate(api, actor0, localFrame0, actor1, localFrame1);
}

