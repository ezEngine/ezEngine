#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

class ezPx6DOFJointComponent;

using ezPxGrabObjectComponentManager = ezComponentManagerSimple<class ezPxGrabObjectComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxGrabObjectComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxGrabObjectComponent, ezPxComponent, ezPxGrabObjectComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxGrabObjectComponent

public:
  ezPxGrabObjectComponent();
  ~ezPxGrabObjectComponent();

  /// \brief Checks whether there is a object that could be picked up.
  bool FindNearbyObject(ezPxDynamicActorComponent*& out_pActorToGrab, ezTransform& out_LocalGrabPoint) const;

  /// \brief Tries to find an object to pick up and do so.
  bool GrabNearbyObject(); // [ scriptable ]

  /// \brief Returns whether an object is currently being held.
  bool HasObjectGrabbed() const; // [ scriptable ]

  /// \brief The grabbed object is dropped in place.
  void DropGrabbedObject(); // [ scriptable ]

  /// \brief Throws the held object away.
  void ThrowGrabbedObject(const ezVec3& vRelativeDir); // [ scriptable ]

  /// \brief Similar to DropGrabbedObject() but additionally posts the event message ezMsgPhysicsJointBroke.
  ///
  /// This can be used to inform other code that the object was ripped from the hands of the player.
  void BreakObjectGrab(); // [ scriptable ]

  /// If the held actor is pushed out of the hands farther than this, the 'joint' breaks. Set to zero to disable this feature.
  float m_fBreakDistance = 0.5f; // [ property ]

  /// The stiffness of the joint to pull the object towards the player's hands.
  /// Careful, too large values mean the held object can push objects that the player itself cannot push.
  float m_fSpringStiffness = 50.0f; // [ property ]

  /// The damping of the joint, to prevent oscillation when moving around.
  float m_fSpringDamping = 10.0f; // [ property ]

  /// How far grab points are allowed to be away to pick them
  float m_fMaxGrabPointDistance = 2.0f; // [ property ]

  /// The collision layer to use for the raycast.
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// If non-zero, the player can pick up objects that have no ezGrabbableItemComponent, if their mass is below this value.
  float m_fAllowGrabAnyObjectWithMass = 20.0f; // [ property ]

  void SetAttachToReference(const char* szReference); // [ property ]

  /// Which other game object to attach the grabbed object to.
  /// It is expected to hold a kinematic ezPxDynamicActorComponent that an ezPxJointComponent can be attached to.
  ezGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject();

  ezPxDynamicActorComponent* FindGrabbableActor() const;
  ezPxDynamicActorComponent* GetAttachToActor() const;
  ezResult DetermineGrabPoint(ezPxDynamicActorComponent* pActor, ezTransform& out_LocalGrabPoint) const;
  void AdjustGrabbedActor(ezPxDynamicActorComponent* pActor);
  void CreateJoint(ezPxDynamicActorComponent* pParent, ezPxDynamicActorComponent* pChild);
  void DetectDistanceViolation(ezPxDynamicActorComponent* pGrabbedActor, ezPx6DOFJointComponent* pJoint);
  bool IsCharacterStandingOnObject(ezGameObjectHandle hActorToGrab) const;

  ezComponentHandle m_hGrabbedActor;
  bool m_bGrabbedActorGravity = true;
  float m_fGrabbedActorMass = 0.0f;
  ezUInt32 m_uiGrabbedActorPosIterations;
  ezUInt32 m_uiGrabbedActorVelIterations;

  ezComponentHandle m_hJoint;
  ezTime m_LastValidTime;
  ezTransform m_ChildAnchorLocal;
  ezComponentHandle m_hCharacterProxyComponent;

private:
  const char* DummyGetter() const { return nullptr; }
};
