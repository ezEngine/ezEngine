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

struct EZ_PHYSXPLUGIN_DLL ezPxActorLockingFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    FreezePositionX = EZ_BIT(0),
    FreezePositionY = EZ_BIT(1),
    FreezePositionZ = EZ_BIT(2),
    FreezeRotationX = EZ_BIT(3),
    FreezeRotationY = EZ_BIT(4),
    FreezeRotationZ = EZ_BIT(5),

    Default = 0
  };

  struct Bits
  {
    StorageType FreezePositionX : 1;
    StorageType FreezePositionY : 1;
    StorageType FreezePositionZ : 1;
    StorageType FreezeRotationX : 1;
    StorageType FreezeRotationY : 1;
    StorageType FreezeRotationZ : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxActorLockingFlags);

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxDynamicActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDynamicActorComponent, ezPxActorComponent, ezPxDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxDynamicActorComponent

public:
  ezPxDynamicActorComponent();
  ~ezPxDynamicActorComponent();

  physx::PxRigidDynamic* GetPxActor() const { return m_pActor; }

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);                               // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);                                   // [ message ]

  bool GetKinematic() const { return m_bKinematic; }                                   // [ property ]
  void SetKinematic(bool b);                                                           // [ property ]

  bool GetDisableGravity() const { return m_bDisableGravity; }                         // [ property ]
  void SetDisableGravity(bool b);                                                      // [ property ]

  bool GetContinuousCollisionDetection() const { return m_bCCD; }                      // [ property ]
  void SetContinuousCollisionDetection(bool b);                                        // [ property ]

  float GetMass() const { return m_fMass; }                                            // [ property ]
  void SetMass(float fMass);                                                           // [ property ]

  float m_fDensity = 1.0f;                                                             // [ property ]
  float m_fLinearDamping = 0.1f;                                                       // [ property ]
  float m_fAngularDamping = 0.05f;                                                     // [ property ]
  float m_fMaxContactImpulse = 1000000.0f;                                             // [ property ]

  ezVec3 GetLocalCenterOfMass() const;                                                 // [ scriptable ]
  ezVec3 GetGlobalCenterOfMass() const;                                                // [ scriptable ]

  void AddLinearForce(const ezVec3& vForce);                                           // [ scriptable ]
  void AddLinearImpulse(const ezVec3& vImpulse);                                       // [ scriptable ]
  void AddAngularForce(const ezVec3& vForce);                                          // [ scriptable ]
  void AddAngularImpulse(const ezVec3& vImpulse);                                      // [ scriptable ]

  void SetLockingFlags(ezBitflags<ezPxActorLockingFlags> flags);                       // [ property ]
  ezBitflags<ezPxActorLockingFlags> GetLockingFlags() const { return m_LockingFlags; } // [ property ]

protected:
  bool FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const;

  physx::PxRigidDynamic* m_pActor = nullptr;

private:
  float m_fMass = 0.0f;
  bool m_bDisableGravity = false;
  bool m_bKinematic = false;
  bool m_bCCD = false;
  ezBitflags<ezPxActorLockingFlags> m_LockingFlags;

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
