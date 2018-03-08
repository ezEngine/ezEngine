#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

class ezPxDynamicActorComponent;
struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponentManager : public ezComponentManager<ezPxDynamicActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxDynamicActorComponentManager(ezWorld* pWorld);
  ~ezPxDynamicActorComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxDynamicActorComponent;

  void UpdateKinematicActors();
  void UpdateDynamicActors(ezArrayPtr<physx::PxActor*> activeActors);

  void UpdateMaxDepenetrationVelocity(float fMaxVelocity);

  ezDynamicArray<ezPxDynamicActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDynamicActorComponent, ezPxActorComponent, ezPxDynamicActorComponentManager);

public:
  ezPxDynamicActorComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg);
  void AddForceAtPos(ezMsgPhysicsAddForce& msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties

  bool GetKinematic() const { return m_bKinematic; }
  void SetKinematic(bool b);

  bool GetDisableGravity() const { return m_bDisableGravity; }
  void SetDisableGravity(bool b);

  float GetMass() const { return m_fMass; }
  void SetMass(float fMass);

  float m_fDensity;
  float m_fLinearDamping;
  float m_fAngularDamping;
  float m_fMaxContactImpulse;

public:
  virtual void OnSimulationStarted() override;

  virtual void Deinitialize() override;

  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  ezVec3 GetLocalCenterOfMass() const;
  ezVec3 GetGlobalCenterOfMass() const;

  /// \todo Turn these into messages ?

  void AddLinearForce(const ezVec3& vForce);
  void AddLinearImpulse(const ezVec3& vImpulse);

  void AddAngularForce(const ezVec3& vForce);
  void AddAngularImpulse(const ezVec3& vImpulse);

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  physx::PxRigidDynamic* m_pActor;

private:
  float m_fMass;
  bool m_bDisableGravity;
  bool m_bKinematic;

  ezPxUserData m_UserData;
};
