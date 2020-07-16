#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxRevoluteJointComponentManager = ezComponentManager<class ezPxRevoluteJointComponent, ezBlockStorageType::Compact>;

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
  virtual void CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRevoluteJointComponent

public:
  ezPxRevoluteJointComponent();
  ~ezPxRevoluteJointComponent();

  void SetLimitMode(ezPxJointLimitMode::Enum mode);                     // [ property ]
  ezPxJointLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  void SetUpperLimitAngle(ezAngle f);                         // [ property ]
  ezAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  void SetSpringStiffness(float f);                               // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; } // [ property ]

  void SetSpringDamping(float f);                             // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; } // [ property ]

  bool m_bEnableDrive = false;        // [ property ]
  bool m_bEnableDriveBraking = false; // [ property ]

  void SetDriveVelocity(float f);                             // [ property ]
  float GetDriveVelocity() const { return m_fDriveVelocity; } // [ property ]

  void SetDriveTorque(float f);                              // [ property ]
  float GetDriveTorque() const { return m_fMaxDriveTorque; } // [ property ]

protected:
  ezEnum<ezPxJointLimitMode> m_LimitMode;
  ezAngle m_LowerLimit;
  ezAngle m_UpperLimit;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;
  float m_fDriveVelocity = 0;    // [ property ]
  float m_fMaxDriveTorque = 100; // [ property ]

  void ApplyLimits();
  void ApplyDrive();
};
