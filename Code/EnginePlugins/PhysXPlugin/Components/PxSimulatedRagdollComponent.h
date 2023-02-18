#pragma once

#include <PhysXPlugin/Components/PxRagdollComponent.h>

using ezPxSimulatedRagdollComponentManager = ezComponentManagerSimple<class ezPxSimulatedRagdollComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxSimulatedRagdollComponent : public ezPxRagdollComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSimulatedRagdollComponent, ezPxRagdollComponent, ezPxSimulatedRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRagdollComponent

public:
  ezPxSimulatedRagdollComponent();
  ~ezPxSimulatedRagdollComponent();

protected:
  virtual void ClearPhysicsObjects() override;
  virtual void SetupPxBasics(physx::PxPhysics* pPxApi, ezPhysXWorldModule* pPxModule) override;
  virtual void FinishSetupLimbs() override;
  virtual void CreateLimbBody(physx::PxPhysics* pPxApi, const LimbConfig& parentLimb, LimbConfig& thisLimb) override;
  virtual void CreateLimbJoint(physx::PxPhysics* pPxApi, const ezSkeletonJoint& thisJoint, physx::PxRigidBody* pPxParentBody, const ezTransform& parentFrame, physx::PxRigidBody* pPxThisBody, const ezTransform& thisFrame) override;
  virtual void WakeUp() override;
  virtual bool IsSleeping() const override;

  physx::PxArticulation* m_pPxArticulation = nullptr;
};
