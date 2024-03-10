#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxPrismaticJointComponentManager = ezComponentManager<class ezPxPrismaticJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxPrismaticJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxPrismaticJointComponent, ezPxJointComponent, ezPxPrismaticJointComponentManager);

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
  // ezPxPrismaticJointComponent

public:
  ezPxPrismaticJointComponent();
  ~ezPxPrismaticJointComponent();

  void SetLimitMode(ezPxJointLimitMode::Enum mode);                     // [ property ]
  ezPxJointLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  void SetSpringStiffness(float f);                                     // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; }       // [ property ]

  void SetSpringDamping(float f);                                       // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; }           // [ property ]

  virtual void ApplySettings() final override;

protected:
  ezEnum<ezPxJointLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;
};
