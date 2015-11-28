#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>

typedef ezComponentManagerAbstract<class ezPxJointComponent> ezPxJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxJointComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxJointComponent, ezPhysXComponent, ezPxJointComponentManager);

public:
  ezPxJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override;

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

  PxJoint* SetupJoint();

  virtual PxJoint* CreateJointType(PxPhysics& api, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1) = 0;

  PxJoint* m_pJoint;
};


