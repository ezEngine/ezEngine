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


  void SetParentActor(const char* szReference);
  void SetChildActor(const char* szReference);


protected:


  // ************************************* FUNCTIONS *****************************

public:

  void SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB);

protected:

  ezResult FindParentBody(physx::PxRigidActor*& pActor);
  ezResult FindChildBody(physx::PxRigidActor*& pActor);

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) = 0;

  ezGameObjectHandle m_hActorA;
  ezGameObjectHandle m_hActorB;

  // UserFlag0 specifies whether m_localFrameA is already set
  ezTransform m_localFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  ezTransform m_localFrameB;

  physx::PxJoint* m_pJoint;

private:
  const char* DummyGetter() const { return nullptr; }
};


