#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

class EZ_PHYSXPLUGIN_DLL ezPxJointComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, ezPxComponent);

public:
  ezPxJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:
  float m_fBreakForce;
  float m_fBreakTorque;
  bool m_bPairCollision;


protected:


  // ************************************* FUNCTIONS *****************************

public:


protected:
  virtual void Deinitialize() override;

  physx::PxJoint* SetupJoint();

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) = 0;

  physx::PxJoint* m_pJoint;
};


