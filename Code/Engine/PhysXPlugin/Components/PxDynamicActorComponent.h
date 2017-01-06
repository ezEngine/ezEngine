#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>

class ezPxDynamicActorComponent;

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponentManager : public ezComponentManager<ezPxDynamicActorComponent, true>
{
public:
  ezPxDynamicActorComponentManager(ezWorld* pWorld);
  ~ezPxDynamicActorComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxDynamicActorComponent;

  void UpdateKinematicActors();
  void UpdateDynamicActors(ezArrayPtr<const PxActiveTransform> activeTransforms);

  ezDynamicArray<ezPxDynamicActorComponent*> m_KinematicActors;
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

  PxRigidDynamic* GetActor() const { return m_pActor; }

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  PxRigidDynamic* m_pActor;

private:
  bool m_bDisableGravity;
  bool m_bKinematic;
};
