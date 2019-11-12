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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void Deinitialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxDynamicActorComponent Interface
public:
  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

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

  float m_fDensity;           // [ property ]
  float m_fLinearDamping;     // [ property ]
  float m_fAngularDamping;    // [ property ]
  float m_fMaxContactImpulse; // [ property ]

  ezVec3 GetLocalCenterOfMass() const;  // [ scriptable ]
  ezVec3 GetGlobalCenterOfMass() const; // [ scriptable ]

  void AddLinearForce(const ezVec3& vForce);      // [ scriptable ]
  void AddLinearImpulse(const ezVec3& vImpulse);  // [ scriptable ]
  void AddAngularForce(const ezVec3& vForce);     // [ scriptable ]
  void AddAngularImpulse(const ezVec3& vImpulse); // [ scriptable ]

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  physx::PxRigidDynamic* m_pActor;

private:
  float m_fMass;
  bool m_bDisableGravity;
  bool m_bKinematic;
  bool m_bCCD;

  ezPxUserData m_UserData;
};
