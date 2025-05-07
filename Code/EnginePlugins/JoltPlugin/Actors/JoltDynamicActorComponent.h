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

/// \brief Turns an object into a fully physically simulated movable object.
///
/// By default dynamic objects push each other and collide with static obstacles.
/// They can be switched to be "kinematic" in which case they act like static objects, but can be moved around and will
/// still push aside other dynamic objects.
///
/// Dynamic actors can also be moved through forces and impulses.
///
/// Dynamic actors must be made up of convex collision meshes. They cannot use concave meshes.
class EZ_JOLTPLUGIN_DLL ezJoltDynamicActorComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltDynamicActorComponent, ezJoltActorComponent, ezJoltDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltDynamicActorComponent

public:
  ezJoltDynamicActorComponent();
  ~ezJoltDynamicActorComponent();

  /// \brief Adds a physics impulse to this body at the given location.
  ///
  /// An impulse is a force that is applied only once, e.g. a sudden push.
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg); // [ message ]

  /// \brief Adds a physics force to this body at the given location.
  ///
  /// A force is something that applies a constant push, for example wind that blows on an object over a longer duration.
  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg); // [ message ]

  /// \brief Turns the actor into a 'kinematic' actor.
  ///
  /// If an actor is kinematic, it isn't fully simulated anymore, meaning forces do not affect it further.
  /// Thus it does not fall down, it can't be pushed by other actors, it is mostly like a static actor.
  ///
  /// However, it can be moved programmatically. If other code changes the position of this object,
  /// it will move to that location and while doing so, it pushes other dynamic actors out of the way.
  ///
  /// Kinematic actors are typically used for moving platforms, doors and other objects that need to move and push aside
  /// dynamic actors.
  void SetKinematic(bool b);                         // [ property ]
  bool GetKinematic() const { return m_bKinematic; } // [ property ]

  /// \brief Adjusts how strongly gravity affects this actor. Has no effect for kinematic actors.
  ///
  /// Set this to zero to fully disable gravity.
  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  void SetSurfaceFile(ezStringView sFile);                    // [ property ]
  ezStringView GetSurfaceFile() const;                        // [ property ]

  /// \brief If enabled, a more precise simulation method is used, preventing fast moving actors from tunneling through walls.
  /// This comes at an extra performance cost.
  bool m_bCCD = false; // [ property ]

  /// \brief If true, the actor will not be simulated right away, but only after another actor starts interacting with it.
  /// This is a performance optimization to prevent performance spikes after loading a level.
  bool m_bStartAsleep = false; // [ property ]

  /// \brief Whether this actor is allowed to go to sleep. Disabling sleeping will come with a performance impact and
  /// should only be done in very rare cases.
  bool m_bAllowSleeping = true; // [ property ]

  ezUInt8 m_uiWeightCategory;   // [ property ]
  float m_fWeightValue = 1.0f;  // [ property ]

  /// \brief How much to dampen linear motion. The higher the value, the quicker a moving object comes to rest.
  float m_fLinearDamping = 0.1f; // [ property ]

  /// \brief How much to dampen angular motion. The higher the value, the quicker a rotating object comes to rest.
  float m_fAngularDamping = 0.05f; // [ property ]

  /// \brief Which surface to use for all shapes. The surface defines various physical properties and interactions.
  ezSurfaceResourceHandle m_hSurface; // [ property ]

  /// \brief What reactions should be triggered when an actor gets into contact with another.
  ezBitflags<ezOnJoltContact> m_OnContact; // [ property ]

  /// \brief A local offset for the center of mass. \see SetUseCustomCoM()
  ezVec3 m_vCenterOfMass = ezVec3::MakeZero(); // [ property ]

  /// \brief Whether a custom center-of-mass shall be used.
  void SetUseCustomCoM(bool b) { SetUserFlag(0, b); }     // [ property ]
  bool GetUseCustomCoM() const { return GetUserFlag(0); } // [ property ]

  /// \brief Adds a linear force to the center-of-mass of this actor. Unless there are other constraints, this would push the object, but not introduce any rotation.
  void AddLinearForce(const ezVec3& vForce); // [ scriptable ]

  /// \brief Adds a linear impulse to the center-of-mass of this actor. Unless there are other constraints, this would push the object, but not introduce any rotation.
  void AddLinearImpulse(const ezVec3& vImpulse); // [ scriptable ]

  /// \brief Adds an angular force to the center-of-mass of this actor. Unless there are other constraints, this would make the object rotate, but not move away.
  void AddAngularForce(const ezVec3& vForce); // [ scriptable ]

  /// \brief Adds an angular impulse to the center-of-mass of this actor. Unless there are other constraints, this would make the object rotate, but not move away.
  void AddAngularImpulse(const ezVec3& vImpulse); // [ scriptable ]

  /// \brief Should be called by components that add Jolt constraints to this body.
  ///
  /// All registered components receive ezJoltMsgDisconnectConstraints in case the body is deleted.
  /// It is necessary to react to that by removing the Jolt constraint, otherwise Jolt will crash during the next update.
  void AddConstraint(ezComponentHandle hComponent);

  /// \brief Should be called when a constraint is removed (though not strictly required) to prevent unnecessary message sending.
  void RemoveConstraint(ezComponentHandle hComponent);

  /// \brief Returns the actual mass of this actor which is either the user defined mass or has been calculated from density.
  /// For kinematic actors this function will return 0.
  float GetMass() const;


protected:
  const ezJoltMaterial* GetJoltMaterial() const;

  float GetWeightValue() const { return m_fWeightValue; }
  void SetWeightValue_Scale(float fValue);
  void SetWeightValue_Mass(float fValue);
  void SetWeightValue_Density(float fValue);

  bool m_bKinematic = false;
  float m_fGravityFactor = 1.0f; // [ property ]

  ezSmallArray<ezComponentHandle, 1> m_Constraints;
};
