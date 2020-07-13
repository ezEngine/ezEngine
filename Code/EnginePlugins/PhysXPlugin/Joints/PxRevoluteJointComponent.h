#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxRevoluteJointComponent, ezBlockStorageType::Compact> ezPxRevoluteJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxRevoluteJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRevoluteJointComponent, ezPxJointComponent, ezPxRevoluteJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRevoluteJointComponent

public:
  ezPxRevoluteJointComponent();
  ~ezPxRevoluteJointComponent();

  bool m_bLimitRotation = false;      // [ property ]
  bool m_bEnableDrive = false;        // [ property ]
  bool m_bEnableDriveBraking = false; // [ property ]

  ezAngle m_LowerLimit; // [ property ]
  ezAngle m_UpperLimit; // [ property ]

  float m_fDriveVelocity = 0;    // [ property ]
  float m_fMaxDriveTorque = 100; // [ property ]
};
