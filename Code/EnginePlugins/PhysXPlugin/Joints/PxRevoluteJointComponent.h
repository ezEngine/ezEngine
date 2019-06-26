#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxRevoluteJointComponent, ezBlockStorageType::Compact> ezPxRevoluteJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxRevoluteJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRevoluteJointComponent, ezPxJointComponent, ezPxRevoluteJointComponentManager);

public:
  ezPxRevoluteJointComponent();
  ~ezPxRevoluteJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  bool m_bLimitRotation = false;
  bool m_bEnableDrive = false;
  bool m_bEnableDriveBraking = false;

  ezAngle m_LowerLimit;
  ezAngle m_UpperLimit;

  float m_fDriveVelocity = 0;


  // ************************************* FUNCTIONS *****************************

public:

protected:

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;
};


