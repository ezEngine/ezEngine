#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxSphericalJointComponentManager = ezComponentManager<class ezPxSphericalJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxSphericalJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSphericalJointComponent, ezPxJointComponent, ezPxSphericalJointComponentManager);


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
  // ezPxSphericalJointComponent

public:
  ezPxSphericalJointComponent();
  ~ezPxSphericalJointComponent();

  void SetLimitMode(ezPxJointLimitMode::Enum mode);                     // [ property ]
  ezPxJointLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  ezAngle GetConeLimitY() const { return m_ConeLimitY; }                // [ property ]
  void SetConeLimitY(ezAngle v);                                        // [ property ]

  ezAngle GetConeLimitZ() const { return m_ConeLimitZ; }                // [ property ]
  void SetConeLimitZ(ezAngle v);                                        // [ property ]

  void SetSpringStiffness(float f);                                     // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; }       // [ property ]

  void SetSpringDamping(float f);                                       // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; }           // [ property ]

  virtual void ApplySettings() final override;

protected:
  ezEnum<ezPxJointLimitMode> m_LimitMode;
  ezAngle m_ConeLimitY;
  ezAngle m_ConeLimitZ;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;
};
