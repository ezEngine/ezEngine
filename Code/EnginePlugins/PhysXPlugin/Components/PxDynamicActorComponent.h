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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxDynamicActorComponent

public:
  ezPxDynamicActorComponent();
  ~ezPxDynamicActorComponent();

  physx::PxRigidDynamic* GetPxActor() const { return m_pActor; }

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg); // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& msg);     // [ message ]

  bool GetKinematic() const { return m_bKinematic; } // [ property ]
  void SetKinematic(bool b);                         // [ property ]

  bool GetDisableGravity() const { return m_bDisableGravity; } // [ property ]
  void SetDisableGravity(bool b);                              // [ property ]

  bool GetContinuousCollisionDetection() const { return m_bCCD; } // [ property ]
  void SetContinuousCollisionDetection(bool b);                   // [ property ]

  float GetMass() const { return m_fMass; } // [ property ]
  void SetMass(float fMass);                // [ property ]

  float m_fDensity = 1.0f;                 // [ property ]
  float m_fLinearDamping = 0.1f;           // [ property ]
  float m_fAngularDamping = 0.05f;         // [ property ]
  float m_fMaxContactImpulse = 1000000.0f; // [ property ]

  ezVec3 GetLocalCenterOfMass() const;  // [ scriptable ]
  ezVec3 GetGlobalCenterOfMass() const; // [ scriptable ]

  void AddLinearForce(const ezVec3& vForce);      // [ scriptable ]
  void AddLinearImpulse(const ezVec3& vImpulse);  // [ scriptable ]
  void AddAngularForce(const ezVec3& vForce);     // [ scriptable ]
  void AddAngularImpulse(const ezVec3& vImpulse); // [ scriptable ]

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  physx::PxRigidDynamic* m_pActor = nullptr;

private:
  float m_fMass = 0.0f;
  bool m_bDisableGravity = false;
  bool m_bKinematic = false;
  bool m_bCCD = false;

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
