#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxPrismaticJointComponent, ezBlockStorageType::Compact> ezPxPrismaticJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxPrismaticJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxPrismaticJointComponent, ezPxJointComponent, ezPxPrismaticJointComponentManager);

public:
  ezPxPrismaticJointComponent();
  ~ezPxPrismaticJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:
  void SetLowerLimitDistance(float f);
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; }

  void SetUpperLimitDistance(float f);
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; }

  void SetSpringStiffness(float f);
  float GetSpringStiffness() const { return m_fSpringStiffness; }

  void SetSpringDamping(float f);
  float GetSpringDamping() const { return m_fSpringDamping; }

protected:
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fSpringStiffness = 0;
  float m_fSpringDamping = 0;

  void ApplyLimits(physx::PxJoint* pJoint);

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;
};
