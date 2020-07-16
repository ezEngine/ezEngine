#include <PhysXPluginPCH.h>

#include <PhysXPlugin/Joints/PxFixedJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxFixedJointComponent, 1, ezComponentMode::Static);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxFixedJointComponent::ezPxFixedJointComponent() = default;
ezPxFixedJointComponent::~ezPxFixedJointComponent() = default;

void ezPxFixedJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxFixedJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxFixedJointComponent);
