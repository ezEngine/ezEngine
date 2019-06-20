#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

class EZ_PHYSXPLUGIN_DLL ezPxJointComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, ezPxComponent);

public:
  ezPxJointComponent();
  ~ezPxJointComponent();

  virtual void OnSimulationStarted() override;
  virtual void Deinitialize() override;

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

  void SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB);

protected:

  void FindActorsInHierarchy();

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) = 0;

  ezGameObjectHandle m_hActorA;
  ezGameObjectHandle m_hActorB;

  ezTransform m_localFrameA;
  ezTransform m_localFrameB;

  physx::PxJoint* m_pJoint;
};


