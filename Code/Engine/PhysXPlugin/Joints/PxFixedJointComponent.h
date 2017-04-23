#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxFixedJointComponent, ezBlockStorageType::Compact> ezPxFixedJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxFixedJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxFixedJointComponent, ezPxJointComponent, ezPxFixedJointComponentManager);

public:
  ezPxFixedJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:



  // ************************************* FUNCTIONS *****************************

public:

protected:

  virtual void OnSimulationStarted() override;

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;
};


