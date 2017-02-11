#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

class ezPxDynamicActorComponent;

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponentManager : public ezComponentManager<ezPxDynamicActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxDynamicActorComponentManager(ezWorld* pWorld);
  ~ezPxDynamicActorComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxDynamicActorComponent;

  void UpdateKinematicActors();
  void UpdateDynamicActors(ezArrayPtr<const physx::PxActiveTransform> activeTransforms);

  ezDynamicArray<ezPxDynamicActorComponent*> m_KinematicActorComponents;
};

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDynamicActorComponent, ezPxActorComponent, ezPxDynamicActorComponentManager);

public:
  ezPxDynamicActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  bool GetKinematic() const { return m_bKinematic; }
  void SetKinematic(bool b);

  bool GetDisableGravity() const { return m_bDisableGravity; }
  void SetDisableGravity(bool b);

  float m_fMass;
  float m_fDensity;
  float m_fLinearDamping;
  float m_fAngularDamping;

public:
  virtual void OnSimulationStarted() override;

  virtual void Deinitialize() override;

  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  ezVec3 GetLocalCenterOfMass() const;
  ezVec3 GetGlobalCenterOfMass() const;

  void AddLinearForce(const ezVec3& vForce);
  void AddLinearImpulse(const ezVec3& vImpulse);

  void AddAngularForce(const ezVec3& vForce);
  void AddAngularImpulse(const ezVec3& vImpulse);

  void AddForceAtPos(const ezVec3& vForce, const ezVec3& vPos);
  void AddImpulseAtPos(const ezVec3& vImpulse, const ezVec3& vPos);

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  physx::PxRigidDynamic* m_pActor;

private:
  bool m_bDisableGravity;
  bool m_bKinematic;

  ezPxUserData m_UserData;
};
