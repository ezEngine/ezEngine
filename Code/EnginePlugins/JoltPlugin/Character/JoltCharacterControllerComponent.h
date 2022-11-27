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

  struct ContactPoint
  {
    float m_fCastFraction = 0.0f;
    ezVec3 m_vPosition;
    ezVec3 m_vSurfaceNormal;
    ezVec3 m_vContactNormal;
    JPH::BodyID m_BodyID;
    JPH::SubShapeID m_SubShapeID;
  };

  /// \brief Checks what the ground state would be at the given position. If there is any contact, returns the most interesting contact point.
  // GroundState DetermineGroundState(ContactPoint& out_Contact, const ezVec3& vFootPosition, float fAllowedStepUp, float fAllowedStepDown, float fCylinderRadius) const;

  /// \brief Checks whether the character could stand at the target position with the desired height without intersecting other geometry
  // bool TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewShapeHeight) const;

  /// \brief Checks whether the character can be resized to the new height without intersecting other geometry.
  // bool CanResize(float fNewShapeHeight) const;

  void SetObjectToIgnore(ezUInt32 uiObjectFilterID);
  void ClearObjectToIgnore();

public:                                               // [ properties ]
  ezUInt8 m_uiCollisionLayer = 0;                     // [ property ]
  ezUInt8 m_uiPresenceCollisionLayer = 0;             // [ property ]
  ezBitflags<ezJoltCharacterDebugFlags> m_DebugFlags; // [ property ]

  /// \brief The maximum slope that the character can walk up.
  void SetMaxClimbingSlope(ezAngle slope);                           // [ property ]
  ezAngle GetMaxClimbingSlope() const { return m_MaxClimbingSlope; } // [ property ]

  /// \brief The mass with which the character will push down on objects that it is standing on.
  void SetMass(float fMass);                // [ property ]
  float GetMass() const { return m_fMass; } // [ property ]

  /// \brief The strength with which the character will push against objects that it is running into.
  void SetStrength(float fStrength);                // [ property ]
  float GetStrength() const { return m_fStrength; } // [ property ]

private:                                            // [ properties ]
  ezAngle m_MaxClimbingSlope = ezAngle::Degree(45); // [ property ]
  float m_fMass = 70.0f;                            // [ property ]
  float m_fStrength = 500.0f;                       // [ property ]

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

  struct ShapeContacts
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Default = 0,
      FlatGround = EZ_BIT(0),
      SteepGround = EZ_BIT(1),
      Ceiling = EZ_BIT(2),
    };

    struct Bits
    {
      StorageType FlatGround : 1;
      StorageType SteepGround : 1;
      StorageType Ceiling : 1;
    };
  };

  /// \brief Looks at all the contact points and determines what kind of contacts they are.
  ///
  /// maxSlopeAngle is used to determine whether there are steep or flat contacts.
  /// vCenterPos (and -maxSlopeAngle) are used to determine whether there are ground/ceiling contacts.
  /// If out_pBestGroundContact != nullptr, an index to some contact point may be returned.
  /// * if there is any flat enough contact, the closest one of those is returned
  /// * if there are no flat contacts, the flattest of the steep contacts is returned
  /// * otherwise an invalid index
  // static ezBitflags<ShapeContacts> ClassifyContacts(const ezDynamicArray<ContactPoint>& contacts, ezAngle maxSlopeAngle, const ezVec3& vCenterPos, ezUInt32* out_pBestGroundContact);

  using ContactFilter = ezDelegate<bool(const ContactPoint&)>;

  /// \brief Returns the index or ezInvalidIndex of the contact point whose normal is closest to vNormal.
  ///
  /// \param filter allows to ignore contact points by other criteria, such as position.
  // static ezUInt32 FindFlattestContact(const ezDynamicArray<ContactPoint>& contacts, const ezVec3& vNormal, ContactFilter filter);

  void VisualizeContact(const ContactPoint& contact, const ezColor& color) const;
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
