#pragma once

#include <PhysXPlugin/Components/PxRagdollComponent.h>

using ezPxControlledRagdollComponentManager = ezComponentManagerSimple<class ezPxControlledRagdollComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxControlledRagdollComponent : public ezPxRagdollComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxControlledRagdollComponent, ezPxRagdollComponent, ezPxControlledRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRagdollComponent

public:
  ezPxControlledRagdollComponent();
  ~ezPxControlledRagdollComponent();

protected:
  virtual void CreateLimbBody(physx::PxPhysics* pPxApi, const LimbConfig& parentLimb, LimbConfig& thisLimb) override;
  virtual void CreateLimbJoint(physx::PxPhysics* pPxApi, const ezSkeletonJoint& thisJoint, physx::PxRigidBody* pPxParentBody, const ezTransform& parentFrame, physx::PxRigidBody* pPxThisBody, const ezTransform& thisFrame) override;
  virtual void WakeUp() override;
  virtual bool IsSleeping() const override;
};
