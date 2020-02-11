#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxPrismaticJointComponent, ezBlockStorageType::Compact> ezPxPrismaticJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxPrismaticJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxPrismaticJointComponent, ezPxJointComponent, ezPxPrismaticJointComponentManager);

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
  // ezPxPrismaticJointComponent

public:
  ezPxPrismaticJointComponent();
  ~ezPxPrismaticJointComponent();

  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  void SetSpringStiffness(float f);                               // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; } // [ property ]

  void SetSpringDamping(float f);                             // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; } // [ property ]

protected:
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;

  void ApplyLimits(physx::PxJoint* pJoint);
};
