#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxRevoluteJointComponentManager = ezComponentManager<class ezPxRevoluteJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxRevoluteJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRevoluteJointComponent, ezPxJointComponent, ezPxRevoluteJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual void CreateJointType(
    physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRevoluteJointComponent

public:
  ezPxRevoluteJointComponent();
  ~ezPxRevoluteJointComponent();

  void SetLimitMode(ezPxJointLimitMode::Enum mode);                     // [ property ]
  ezPxJointLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitAngle(ezAngle f);                                   // [ property ]
  ezAngle GetLowerLimitAngle() const { return m_LowerLimit; }           // [ property ]

  void SetUpperLimitAngle(ezAngle f);                                   // [ property ]
  ezAngle GetUpperLimitAngle() const { return m_UpperLimit; }           // [ property ]

  void SetSpringStiffness(float f);                                     // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; }       // [ property ]

  void SetSpringDamping(float f);                                       // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; }           // [ property ]

  void SetDriveMode(ezPxJointDriveMode::Enum mode);                     // [ property ]
  ezPxJointDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  void SetDriveVelocity(float f);                                       // [ property ]
  float GetDriveVelocity() const { return m_fDriveVelocity; }           // [ property ]

  void SetDriveTorque(float f);                                         // [ property ]
  float GetDriveTorque() const { return m_fMaxDriveTorque; }            // [ property ]

  virtual void ApplySettings() final override;

protected:
  ezEnum<ezPxJointLimitMode> m_LimitMode;
  ezAngle m_LowerLimit;
  ezAngle m_UpperLimit;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;
  ezEnum<ezPxJointDriveMode> m_DriveMode;
  float m_fDriveVelocity = 0;
  float m_fMaxDriveTorque = 100;
};
