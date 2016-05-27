#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

struct ezCallDelayedStartMessage;

typedef ezComponentManager<class ezPxFixedJointComponent> ezPxFixedJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxFixedJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxFixedJointComponent, ezPxJointComponent, ezPxFixedJointComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

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

  virtual PxJoint* CreateJointType(PxPhysics& api, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1) override;
};


