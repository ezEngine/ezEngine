#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltDynamicActorComponentManager : public ezComponentManager<class ezJoltDynamicActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltDynamicActorComponentManager(ezWorld* pWorld);
  ~ezJoltDynamicActorComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltDynamicActorComponent;

  void UpdateKinematicActors(ezTime deltaTime);
  void UpdateDynamicActors();

  ezDynamicArray<ezJoltDynamicActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltDynamicActorComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltDynamicActorComponent, ezJoltActorComponent, ezJoltDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltDynamicActorComponent

public:
  ezJoltDynamicActorComponent();
  ~ezJoltDynamicActorComponent();

  ezUInt32 GetJoltBodyID() const { return m_uiJoltBodyID; }

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg); // [ message ]
  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);     // [ message ]

  bool GetKinematic() const { return m_bKinematic; } // [ property ]
  void SetKinematic(bool b);                         // [ property ]

  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  bool m_bCCD = false;                                    // [ property ]
  bool m_bStartAsleep = false;                            // [ property ]
  float m_fMass = 0.0f;                                   // [ property ]
  float m_fDensity = 1.0f;                                // [ property ]
  float m_fLinearDamping = 0.1f;                          // [ property ]
  float m_fAngularDamping = 0.05f;                        // [ property ]
  ezSurfaceResourceHandle m_hSurface;                     // [ property ]
  ezBitflags<ezOnJoltContact> m_OnContact;                // [ property ]
  ezVec3 m_vCenterOfMass = ezVec3::MakeZero();          // [ property ]
  bool GetUseCustomCoM() const { return GetUserFlag(0); } // [ property ]
  void SetUseCustomCoM(bool b) { SetUserFlag(0, b); }     // [ property ]

  void AddLinearForce(const ezVec3& vForce);      // [ scriptable ]
  void AddLinearImpulse(const ezVec3& vImpulse);  // [ scriptable ]
  void AddAngularForce(const ezVec3& vForce);     // [ scriptable ]
  void AddAngularImpulse(const ezVec3& vImpulse); // [ scriptable ]

  /// \brief Should be called by components that add Jolt constraints to this body.
  ///
  /// All registered components receive ezJoltMsgDisconnectConstraints in case the body is deleted.
  /// It is necessary to react to that by removing the Jolt constraint, otherwise Jolt will crash during the next update.
  void AddConstraint(ezComponentHandle hComponent);

  /// \brief Should be called when a constraint is removed (though not strictly required) to prevent unnecessary message sending.
  void RemoveConstraint(ezComponentHandle hComponent);

protected:
  const ezJoltMaterial* GetJoltMaterial() const;

  bool m_bKinematic = false;
  float m_fGravityFactor = 1.0f; // [ property ]

  ezDynamicArray<ezComponentHandle> m_Constraints;
};
