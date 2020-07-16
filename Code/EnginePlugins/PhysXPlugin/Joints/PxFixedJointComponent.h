#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxFixedJointComponentManager = ezComponentManager<class ezPxFixedJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxFixedJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxFixedJointComponent, ezPxJointComponent, ezPxFixedJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezPxFixedJointComponent

protected:
  virtual void CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxFixedJointComponent

public:
  ezPxFixedJointComponent();
  ~ezPxFixedJointComponent();

  virtual void ApplySettings() final override;
};
