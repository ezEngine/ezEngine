#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxFixedJointComponent, ezBlockStorageType::Compact> ezPxFixedJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxFixedJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxFixedJointComponent, ezPxJointComponent, ezPxFixedJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezPxFixedJointComponent

protected:
  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxFixedJointComponent

public:
  ezPxFixedJointComponent();
  ~ezPxFixedJointComponent();
};
