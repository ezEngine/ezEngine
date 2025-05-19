#pragma once

#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

struct ezMsgMoveCharacterController;
struct ezMsgUpdateLocalBounds;

namespace JPH
{
  class CharacterVirtual;
  class TempAllocator;
} // namespace JPH

EZ_DECLARE_FLAGS(ezUInt32, ezJoltCharacterDebugFlags, PrintState, VisShape, VisContacts, VisCasts, VisGroundContact, VisFootCheck);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltCharacterDebugFlags);

/// \brief Base class for character controllers (CC).
///
/// This class provides general functionality for building a character controller.
/// It tries not to implement things that are game specific.
/// It is assumed that most games implement their own character controller to be able to build very specific behavior.
/// The ezJoltDefaultCharacterComponent is an example implementation that shows how this can be achieved on top of this class.
class EZ_JOLTPLUGIN_DLL ezJoltCharacterControllerComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltCharacterControllerComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltCharacterControllerComponent

public:
  ezJoltCharacterControllerComponent();
  ~ezJoltCharacterControllerComponent();

  /// \brief Describes a point where the CC collided with other geometry.
  struct ContactPoint
  {
    float m_fCastFraction = 0.0f;
    ezVec3 m_vPosition;
    ezVec3 m_vSurfaceNormal;
    ezVec3 m_vContactNormal;
    JPH::BodyID m_BodyID;
    JPH::SubShapeID m_SubShapeID;
  };

  /// \brief The CC will move through the given physics body.
  ///
  /// Currently only one such object can be set. This is mainly used to ignore an object that the player is currently carrying,
  /// so that there are no unintended collisions.
  ///
  /// Call ClearObjectToIgnore() to re-enable collisions.
  void SetObjectToIgnore(ezUInt32 uiObjectFilterID);

  /// \see SetObjectToIgnore()
  void ClearObjectToIgnore();

public:
  /// The collision layer determines with which other actors this actor collides. \see ezJoltActorComponent
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// In case a 'presence shape' is used, this defines which geometry the presence bodies collides with.
  ezUInt8 m_uiPresenceCollisionLayer = 0; // [ property ]

  /// What aspects of the CC to visualize.
  ezBitflags<ezJoltCharacterDebugFlags> m_DebugFlags; // [ property ]

  /// \brief The maximum slope that the character can walk up.
  void SetMaxClimbingSlope(ezAngle slope);                           // [ property ]
  ezAngle GetMaxClimbingSlope() const { return m_MaxClimbingSlope; } // [ property ]

  /// \brief The mass with which the character will push down on objects that it is standing on.
  float GetMass() const { return m_fMass; } // [ property ]

  ezUInt8 m_uiWeightCategory = 0;           // [ property ]
  float m_fWeightValue = 1.0f;              // [ property ]

  /// \brief The strength with which the character will push against objects that it is running into.
  void SetStrength(float fStrength);                        // [ property ]
  float GetStrength() const { return m_fStrength; }         // [ property ]

private:
  ezAngle m_MaxClimbingSlope = ezAngle::MakeFromDegree(45); // [ property ]
  float m_fMass = 70.0f;                                    // [ property ]
  float m_fStrength = 500.0f;                               // [ property ]

  float GetWeightValue() const { return m_fWeightValue; }
  void SetWeightValue_Scale(float fValue);
  void SetWeightValue_Mass(float fValue);

protected:
  /// \brief Returns the time delta to use for updating the character. This may differ from the world delta.
  EZ_ALWAYS_INLINE float GetUpdateTimeDelta() const { return m_fUpdateTimeDelta; }

  /// \brief Returns the inverse of update time delta.
  EZ_ALWAYS_INLINE float GetInverseUpdateTimeDelta() const { return m_fInverseUpdateTimeDelta; }

  /// \brief Returns the shape that the character is supposed to use next.
  ///
  /// The desired target state (radius, height, etc) has to be stored somewhere else (e.g. as members in derived classes).
  /// The shape can be cached.
  /// The shape may not get applied to the character, in case this is used by things like TryResize and the next shape is
  /// determined to not fit.
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() = 0;

  /// \brief Returns the radius of the shape. This never changes at runtime.
  virtual float GetShapeRadius() const = 0;

  /// \brief Called up to once per frame, but potentially less often, if physics updates were skipped due to high framerates.
  ///
  /// All shape modifications and moves should only be executed during this step.
  /// The given deltaTime should be used, rather than the world's time diff.
  virtual void UpdateCharacter() = 0;

  /// \brief Gives access to the internally used JPH::CharacterVirtual.
  JPH::CharacterVirtual* GetJoltCharacter() { return m_pCharacter; }
  const JPH::CharacterVirtual* GetJoltCharacter() const { return m_pCharacter; }

  /// \brief Attempts to change the character shape to the new one. Fails if the new shape overlaps with surrounding geometry.
  ezResult TryChangeShape(JPH::Shape* pNewShape);

  /// \brief Moves the character using the given velocity and timestep, making it collide with and slide along obstacles.
  void RawMoveWithVelocity(const ezVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown);

  /// \brief Variant of RawMoveWithVelocity() that takes a direction vector instead.
  void RawMoveIntoDirection(const ezVec3& vDirection);

  /// \brief Variant of RawMoveWithVelocity() that takes a target position instead.
  void RawMoveToPosition(const ezVec3& vTargetPosition);

  /// \brief Teleports the character to the destination position, even if it would get stuck there.
  void TeleportToPosition(const ezVec3& vGlobalFootPos);

  /// \brief If the CC is slightly above the ground, this will move it down so that it touches the ground.
  ///
  /// If within the max distance no ground contact is found, the function does nothing and returns false.
  bool StickToGround(float fMaxDist);

  /// \brief Gathers all contact points that are found by sweeping the shape along a direction
  void CollectCastContacts(ezDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezQuat& qQueryRotation, const ezVec3& vSweepDir) const;

  /// \brief Gathers all contact points of the shape at the target position.
  ///
  /// Use fCollisionTolerance > 0 (e.g. 0.02f) to find contacts with walls/ground that the shape is touching but not penetrating.
  void CollectContacts(ezDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezQuat& qQueryRotation, float fCollisionTolerance) const;

  /// \brief Detects the velocity at the contact point. If it is a dynamic body, a force pushing it away is applied.
  ///
  /// This is mainly used to get the velocity of the kinematic object that a character is standing on.
  /// It can then be incorporated into the movement, such that the character rides along.
  /// If the body at the contact point is dynamic, optionally a force can be applied, simulating that the character's
  /// weight pushes down on it.
  ezVec3 GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce);

  /// \brief Spawns a surface interaction prefab at the given contact point.
  ///
  /// hFallbackSurface is used, if no other surface could be determined from the contact point.
  void SpawnContactInteraction(const ContactPoint& contact, const ezHashedString& sSurfaceInteraction, ezSurfaceResourceHandle hFallbackSurface, const ezVec3& vInteractionNormal = ezVec3(0, 0, 1));

  /// \brief Debug draws the contact point.
  void VisualizeContact(const ContactPoint& contact, const ezColor& color) const;

  /// \brief Debug draws all the contact points.
  void VisualizeContacts(const ezDynamicArray<ContactPoint>& contacts, const ezColor& color) const;

private:
  friend class ezJoltWorldModule;

  void Update(ezTime deltaTime);

  float m_fUpdateTimeDelta = 0.1f;
  float m_fInverseUpdateTimeDelta = 1.0f;
  JPH::CharacterVirtual* m_pCharacter = nullptr;

  void CreatePresenceBody();
  void RemovePresenceBody();
  void MovePresenceBody(ezTime deltaTime);

  ezUInt32 m_uiPresenceBodyID = ezInvalidIndex;

  ezJoltBodyFilter m_BodyFilter;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
